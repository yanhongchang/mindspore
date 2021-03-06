/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <algorithm>
#include "backend/session/ascend_inference_session.h"
#include "frontend/operator/ops.h"
#include "ir/tensor.h"
#include "ir/anf.h"
#include "ir/param_value.h"
#include "runtime/device/kernel_runtime.h"
#include "backend/session/anf_runtime_algorithm.h"
#include "common/utils.h"
#include "common/trans.h"
#include "backend/kernel_compiler/tbe/tbe_python_funcs.h"
#include "utils/config_manager.h"
#include "utils/base_ref_extends.h"

namespace mindspore {
namespace session {
void AscendInferenceSession::LoadInputData(const std::shared_ptr<KernelGraph> &kernel_graph,
                                           const std::vector<tensor::TensorPtr> &inputs_const) const {
  MS_EXCEPTION_IF_NULL(kernel_graph);
  std::vector<tensor::TensorPtr> inputs(inputs_const);
  auto input_nodes = kernel_graph->inputs();

  size_t no_weight_input = 0;
  for (size_t i = 0; i < input_nodes.size(); ++i) {
    tensor::TensorPtr tensor = nullptr;
    if (!input_nodes[i]->isa<Parameter>()) {
      MS_LOG(ERROR) << "Kernel graph inputs have anfnode which is not Parameter";
      continue;
    }
    auto pk_node = input_nodes[i]->cast<ParameterPtr>();
    MS_EXCEPTION_IF_NULL(pk_node);
    auto device_address = AnfAlgo::GetMutableOutputAddr(pk_node, 0);
    MS_EXCEPTION_IF_NULL(device_address);
    if (!AnfAlgo::IsParameterWeight(pk_node)) {
      tensor = inputs[no_weight_input++];
      if (!device_address->SyncHostToDevice(trans::GetRuntimePaddingShape(pk_node, 0),
                                            LongToSize(tensor->data().nbytes()), tensor->data_type(),
                                            tensor->data_c())) {
        MS_LOG(EXCEPTION) << "SyncHostToDevice failed.";
      }
    }
  }
}

GraphId AscendInferenceSession::CompileGraph(NotNull<FuncGraphPtr> func_graph) {
  auto graph_id = AscendSession::CompileGraph(func_graph);
  auto kernel_graph = GetGraph(graph_id);
  MS_EXCEPTION_IF_NULL(kernel_graph);
  // load weight data to device
  auto input_nodes = kernel_graph->inputs();
  for (size_t i = 0; i < input_nodes.size(); ++i) {
    if (!input_nodes[i]->isa<Parameter>()) {
      MS_LOG(ERROR) << "Kernel graph inputs have anfnode which is not Parameter";
      continue;
    }
    auto pk_node = input_nodes[i]->cast<ParameterPtr>();
    MS_EXCEPTION_IF_NULL(pk_node);
    auto device_address = AnfAlgo::GetMutableOutputAddr(pk_node, 0);
    MS_EXCEPTION_IF_NULL(device_address);
    if (AnfAlgo::IsParameterWeight(pk_node)) {
      const auto &param_value = pk_node->default_param();
      MS_EXCEPTION_IF_NULL(param_value);
      auto tensor = std::dynamic_pointer_cast<tensor::Tensor>(param_value->value());
      MS_EXCEPTION_IF_NULL(tensor);
      if (!device_address->SyncHostToDevice(trans::GetRuntimePaddingShape(pk_node, 0),
                                            LongToSize(tensor->data().nbytes()), tensor->data_type(),
                                            tensor->data_c())) {
        MS_LOG(EXCEPTION) << "SyncHostToDevice failed.";
      }
    }
  }
  return graph_id;
}

bool AscendInferenceSession::CheckModelInputs(uint32_t graph_id, const std::vector<tensor::TensorPtr> &inputs) const {
  MS_LOG(INFO) << "Start check client inputs, graph id : " << graph_id;
  auto kernel_graph = GetGraph(graph_id);
  MS_EXCEPTION_IF_NULL(kernel_graph);
  auto kernel_graph_inputs = kernel_graph->inputs();
  size_t no_weight_input = 0;
  vector<ParameterPtr> paras;
  // find parameters of graph inputs
  for (size_t i = 0; i < kernel_graph_inputs.size(); ++i) {
    if (!kernel_graph_inputs[i]->isa<Parameter>()) {
      MS_LOG(ERROR) << "Kernel graph inputs have anfnode which is not Parameter.";
      continue;
    }
    auto parameter = kernel_graph_inputs[i]->cast<ParameterPtr>();
    if (!AnfAlgo::IsParameterWeight(parameter)) {
      paras.push_back(parameter);
    }
  }

  // check inputs
  for (size_t i = 0; i < paras.size(); ++i) {
    // compare input number
    if (paras.size() != inputs.size()) {
      MS_LOG(ERROR) << "Input number is inconsistent. The actual input number [" << inputs.size()
                    << "] but the graph input number is [" << paras.size() << "]";
      MS_LOG(ERROR) << "InputsInfo --" << InputsInfo(paras, inputs);
      return false;
    }
    auto input = inputs[no_weight_input++];
    if (!CompareInput(input, paras[i])) {
      MS_LOG(ERROR) << "Please check the input information.";
      MS_LOG(ERROR) << "InputsInfo --" << InputsInfo(paras, inputs);
      return false;
    }
  }
  return true;
}

bool AscendInferenceSession::CompareInput(const tensor::TensorPtr &input, const ParameterPtr &parameter) const {
  MS_EXCEPTION_IF_NULL(input);
  MS_EXCEPTION_IF_NULL(parameter);
  // compare dims
  auto parameter_shape = AnfAlgo::GetOutputDeviceShape(parameter, 0);

  // compare shape
  auto input_shape = input->shape();
  vector<size_t> trans_input;
  (void)std::transform(input_shape.begin(), input_shape.end(), std::back_inserter(trans_input),
                       [](const int dim) { return static_cast<size_t>(dim); });
  if (trans_input != parameter_shape) {
    MS_LOG(ERROR) << "Input shape is inconsistent. The actual shape is " << PrintInputShape(trans_input)
                  << ", but the parameter shape is " << PrintInputShape(parameter_shape)
                  << ". parameter : " << parameter->DebugString();
    return false;
  }

  // compare data type
  auto kernel_build_info = AnfAlgo::GetSelectKernelBuildInfo(parameter);
  if (input->data_type() != kernel_build_info->GetOutputDeviceType(0)) {
    MS_LOG(ERROR) << "Input data type is inconsistent. The actual data type is " << input->data_type()
                  << ", but the parameter data type is " << kernel_build_info->GetOutputDeviceType(0)
                  << ". parameter : " << parameter->DebugString();
    return false;
  }
  return true;
}

template <typename T>
std::string AscendInferenceSession::PrintInputShape(std::vector<T> shape) const {
  string res = "[";
  for (auto dim : shape) {
    res += " " + std::to_string(dim);
  }
  return res + " ]";
}

std::string AscendInferenceSession::InputsInfo(const std::vector<ParameterPtr> &paras,
                                               const std::vector<tensor::TensorPtr> &inputs) const {
  std::string graph = "graph inputs:{ ";
  for (size_t i = 0; i < paras.size(); ++i) {
    graph += std::to_string(i) + ": dims " + std::to_string(AnfAlgo::GetOutputDeviceShape(paras[i], 0).size()) +
             ", shape " + PrintInputShape(AnfAlgo::GetOutputDeviceShape(paras[i], 0)) + ", data type " +
             std::to_string(AnfAlgo::GetSelectKernelBuildInfo(paras[i])->GetOutputDeviceType(0)) + " }";
  }

  std::string actual = "actual inputs:{ ";
  for (size_t i = 0; i < inputs.size(); ++i) {
    actual += std::to_string(i) + ": dims " + std::to_string(inputs[i]->shape().size()) + ", shape " +
              PrintInputShape(inputs[i]->shape()) + ", data type " + std::to_string(inputs[i]->data_type()) + " }";
  }
  return graph + "   " + actual;
}

}  // namespace session
}  // namespace mindspore
