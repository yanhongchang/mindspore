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

#include <vector>
#include "schema/model_generated.h"
#include "src/kernel_registry.h"
#include "src/runtime/kernel/arm/fp32/pad.h"
#include "include/errorcode.h"
#include "src/runtime/kernel/arm/opclib/errorcode.h"
#include "src/runtime/runtime_api.h"

using mindspore::kernel::KERNEL_ARCH::kCPU;
using mindspore::lite::KernelRegistrar;
using mindspore::lite::RET_ERROR;
using mindspore::lite::RET_NULL_PTR;
using mindspore::lite::RET_OK;
using mindspore::schema::PrimitiveType_Pad;

namespace mindspore::kernel {
namespace {
constexpr int kInputNum = 1;
constexpr int kOutputNum = 1;
constexpr int kInputRank = 4;
constexpr int kPaddingsSize = 8;
}  // namespace

int PadCPUKernel::Init() {
  if (inputs_.size() != kInputNum || outputs_.size() != kOutputNum) {
    MS_LOG(ERROR) << "Pad input size should be " << kInputNum << ", got " << inputs_.size() << ", output size should be"
                  << kOutputNum << ", got " << outputs_.size();
    return RET_ERROR;
  }

  auto input = inputs_.at(0);
  auto output = outputs_.at(0);
  if (input == nullptr || output == nullptr) {
    MS_LOG(ERROR) << "Pad input or output nullptr";
    return RET_NULL_PTR;
  }

  auto rank = input->shape().size();
  if (rank != kInputRank) {
    MS_LOG(ERROR) << "Pad input rank should be " << kInputRank << ", got " << rank;
    return RET_ERROR;
  }

  if (paddings_size_ != kPaddingsSize) {
    MS_LOG(ERROR) << "Pad op paddings size should be 2*input_rank: " << 2 * rank << " but got " << paddings_size_;
    return RET_ERROR;
  }

  for (auto pad : paddings_) {
    if (pad < 0) {
      MS_LOG(ERROR) << "Pad op paddings should be >= 0, but got " << pad;
      return RET_ERROR;
    }
  }
  return RET_OK;
}

int PadImpl(int task_id, LiteParallelGroupEnv *penv, void *cdata) {
  auto padKernel = reinterpret_cast<PadCPUKernel *>(cdata);
  int error_code = padKernel->RunImpl(task_id);
  if (error_code != OPCLIB_OK) {
    MS_LOG(ERROR) << "Pad Run error task_id[" << task_id << "] error_code[" << error_code << "]";
    return RET_ERROR;
  }
  return RET_OK;
}

int PadCPUKernel::RunImpl(int task_id) {
  auto input = inputs_.at(0);
  auto output = outputs_.at(0);

  auto input_data = reinterpret_cast<float *>(input->Data());
  auto output_data = reinterpret_cast<float *>(output->Data());
  auto input_shape = input->shape().data();
  auto output_shape = output->shape().data();

  Pad(input_data, output_data, input_shape, output_shape, paddings_.data(), task_id, context_->threadNum);

  return RET_OK;
}

int PadCPUKernel::Run() {
  auto output = outputs_.at(0);
  int output_size = output->DataSize();

  auto output_data = reinterpret_cast<float *>(output->Data());
  // todo parallel memset to save time
  memset(output_data, 0, output_size * sizeof(float));

  int error_code = LiteBackendParallelLaunch(PadImpl, this, context_->threadNum);
  if (error_code != RET_OK) {
    MS_LOG(ERROR) << "Pad run error, error_code[" << error_code << "]";
    return RET_ERROR;
  }
  return RET_OK;
}
}  // namespace mindspore::kernel
