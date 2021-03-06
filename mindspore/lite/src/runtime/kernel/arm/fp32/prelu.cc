/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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
#include "src/runtime/kernel/arm/fp32/prelu.h"
#include <vector>
#include "schema/model_generated.h"
#include "src/runtime/kernel/arm/opclib/prelu.h"
#include "src/kernel_registry.h"
#include "include/errorcode.h"
#include "src/runtime/runtime_api.h"

using mindspore::kernel::KERNEL_ARCH::kCPU;
using mindspore::lite::KernelRegistrar;
using mindspore::lite::RET_ERROR;
using mindspore::lite::RET_OK;
using mindspore::schema::PrimitiveType_Prelu;

namespace mindspore::kernel {
int PReluCPUKernel::Init() {
  prelu_param_->op_parameter_.thread_num_ = thread_count_;
  return RET_OK;
}

int PReluCPUKernel::DoExcute(int task_id) {
  PRelu(input_data, output_data, prelu_param_, task_id);
  return RET_OK;
}

int PReluRun(int task_id, LiteParallelGroupEnv *penv, void *cdata) {
  auto PReludata = reinterpret_cast<PReluCPUKernel *>(cdata);
  auto ret = PReludata->DoExcute(task_id);
  if (ret != RET_OK) {
    MS_LOG(ERROR) << "PReluRun error task_id[" << task_id << "] error_code[" << ret << "]";
    return RET_ERROR;
  }
  return RET_OK;
}

int PReluCPUKernel::Run() {
  auto input = inputs_.at(0);
  prelu_param_->input_num_ = input->ElementsNum();
  input_data = reinterpret_cast<float *>(input->Data());
  output_data = reinterpret_cast<float *>(outputs_.at(0)->Data());

  auto ret = LiteBackendParallelLaunch(PReluRun, this, prelu_param_->thread_num_);
  if (ret != RET_OK) {
    MS_LOG(ERROR) << "PReluDwRun error: error_code[" << ret << "]";
    return RET_ERROR;
  }
  return RET_OK;
}

kernel::LiteKernel *CpuPReluFp32KernelCreator(const std::vector<lite::tensor::Tensor *> &inputs,
                                              const std::vector<lite::tensor::Tensor *> &outputs,
                                              OpParameter *opParameter, const lite::Context *ctx,
                                              const kernel::KernelKey &desc) {
  if (opParameter == nullptr) {
    MS_LOG(ERROR) << "input opParameter is nullptr!";
    return nullptr;
  }

  auto *kernel = new (std::nothrow) PReluCPUKernel(opParameter, inputs, outputs, ctx);
  if (kernel == nullptr) {
    MS_LOG(ERROR) << "new PReluCPUKernel fail!";
    return nullptr;
  }
  auto ret = kernel->Init();
  if (ret != RET_OK) {
    MS_LOG(ERROR) << "Init kernel failed, name: " << opParameter->name_ << ", type: "
                  << schema::EnumNamePrimitiveType(static_cast<schema::PrimitiveType>(opParameter->type_));
    delete kernel;
    return nullptr;
  }
  return kernel;
}

REG_KERNEL(kCPU, PrimitiveType_Prelu, CpuPReluFp32KernelCreator)
}  // namespace mindspore::kernel

