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

#ifndef MINDSPORE_LITE_SRC_RUNTIME_KERNEL_ARM_OPCLIB_FP32_REDUCE_H_
#define MINDSPORE_LITE_SRC_RUNTIME_KERNEL_ARM_OPCLIB_FP32_REDUCE_H_
#include "src/runtime/kernel/arm/opclib/op_base.h"
#define REDUCE_MAX_AXES_NUM 8

struct ReduceParameter {
  OpParameter op_parameter_;
  bool keep_dims_;
  int axes_[REDUCE_MAX_AXES_NUM];
  int num_axes_;
  int mode_;
};

int ReduceMean(const int outer_size, const int inner_size, const int axis_size, const float *src_data,
               const int *src_shape, float *dst_data, const int tid, const int thread_num);
int ReduceSum(const int outer_size, const int inner_size, const int axis_size, const float *src_data,
              const int *src_shape, float *dst_data, const int tid, const int thread_num);
int ReduceMax(const int outer_size, const int inner_size, const int axis_size, const float *src_data,
              const int *src_shape, float *dst_data, const int tid, const int thread_num);
int ReduceMin(const int outer_size, const int inner_size, const int axis_size, const float *src_data,
              const int *src_shape, float *dst_data, const int tid, const int thread_num);
int ReduceProd(const int outer_size, const int inner_size, const int axis_size, const float *src_data,
               const int *src_shape, float *dst_data, const int tid, const int thread_num);
int ReduceSumSquare(const int outer_size, const int inner_size, const int axis_size, const float *src_data,
                    const int *src_shape, float *dst_data, const int tid, const int thread_num);
#endif  // MINDSPORE_LITE_SRC_RUNTIME_KERNEL_ARM_OPCLIB_FP32_REDUCE_H_
