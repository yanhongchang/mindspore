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

#include "src/ops/ops.h"
#include "include/errorcode.h"
#include "utils/log_adapter.h"
#include "src/ir/tensor.h"

namespace mindspore::lite {
int ArgMax::InferShape(std::vector<tensor::Tensor *> inputs_, std::vector<tensor::Tensor *> outputs_) {
  MS_ASSERT(this->primitive != nullptr);
  auto input = inputs_.front();
  MS_ASSERT(input != nullptr);
  auto output = outputs_.front();
  MS_ASSERT(output != nullptr);
  if (inputs_.size() != kSingleNum || outputs_.size() != kSingleNum) {
    MS_LOG(ERROR) << "tensor number is error.";
  }
  auto argmax_prim = this->primitive->value_as_ArgMax();

  std::vector<int> output_shape(input->shape());
  auto input_shape_size = input->shape().size();
  int axis = argmax_prim->axis() < 0 ? argmax_prim->axis() + input_shape_size : argmax_prim->axis();
  if (axis >= input_shape_size || axis < 0) {
    MS_LOG(ERROR) << "Invalid axis " << argmax_prim->axis() << ", input shape size: " << input_shape_size;
    return RET_PARAM_INVALID;
  }
  if (argmax_prim->topK() == -1) {
    output_shape.erase(output_shape.begin() + axis);
  } else if (argmax_prim->axisType() == 1) {
    output_shape[axis] = argmax_prim->topK();
  }

  output->SetFormat(input->GetFormat());
  output->set_shape(output_shape);
  output->set_data_type(input->data_type());
  return RET_OK;
}
}  // namespace mindspore::lite

