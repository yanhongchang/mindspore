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

#include "src/runtime/kernel/arm/fp32/flatten.h"
#include "schema/model_generated.h"
#include "src/kernel_registry.h"
#include "src/runtime/kernel/arm/opclib/flatten.h"
#include "include/errorcode.h"

using mindspore::kernel::KERNEL_ARCH::kCPU;
using mindspore::lite::KernelRegistrar;
using mindspore::lite::RET_ERROR;
using mindspore::lite::RET_OK;
using mindspore::schema::PrimitiveType_Flatten;

namespace mindspore::kernel {
int FlattenCPUKernel::Init() {
  auto output_shape = outputs_[0]->shape();
  flatten_param_->size = sizeof(float);
  for (int i = 0; i < output_shape.size(); i++) {
    flatten_param_->size *= output_shape[i];
  }
  return RET_OK;
}

int FlattenCPUKernel::ReSize() { return RET_OK; }

int FlattenCPUKernel::Run() {
  auto input = reinterpret_cast<float *>(inputs_[0]->Data());
  auto output = reinterpret_cast<float *>(outputs_[0]->Data());
  Flatten(input, output, flatten_param_);
  return RET_OK;
}

kernel::LiteKernel *CpuFlattenFp32KernelCreator(const std::vector<lite::tensor::Tensor *> &inputs,
                                                const std::vector<lite::tensor::Tensor *> &outputs,
                                                OpParameter *opParameter, const lite::Context *ctx,
                                                const kernel::KernelKey &desc) {
  MS_ASSERT(opParameter != nullptr);
  if (opParameter == nullptr) {
    MS_LOG(ERROR) << "Create kernel failed, opParameter is nullptr, type: PrimitiveType_Flatten. ";
    return nullptr;
  }
  MS_ASSERT(desc.type == schema::PrimitiveType_Flatten);
  auto *kernel = new (std::nothrow) FlattenCPUKernel(opParameter, inputs, outputs, ctx);
  auto ret = kernel->Init();
  if (ret != RET_OK) {
    MS_LOG(ERROR) << "Init kernel failed, name: " << opParameter->name_ << ", type: "
                  << schema::EnumNamePrimitiveType(static_cast<schema::PrimitiveType>(opParameter->type_));
    delete kernel;
    return nullptr;
  }
  return kernel;
}

REG_KERNEL(kCPU, PrimitiveType_Flatten, CpuFlattenFp32KernelCreator)
}  // namespace mindspore::kernel

