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

#ifndef MINDSPORE_LITE_SRC_RUNTIME_KERNEL_ARM_OPCLIB_OPTIMIZED_KERNEL_H_
#define MINDSPORE_LITE_SRC_RUNTIME_KERNEL_ARM_OPCLIB_OPTIMIZED_KERNEL_H_

#include <dlfcn.h>
#ifdef __ANDROID__
#include <asm/hwcap.h>
#include "src/runtime/kernel/arm/opclib/opclib_utils.h"
#endif

#define OPTIMIZE_SHARED_LIBRARY_PATH "liboptimize.so"

class OptimizeModule {
 public:
  OptimizeModule() {
    bool support_optimize_ops = false;

#ifdef __ANDROID__
    int hwcap_type = 16;
    uint32_t hwcap = getHwCap(hwcap_type);
#if defined(__aarch64__)
    if (hwcap & HWCAP_ASIMDDP) {
      printf("Hw cap support SMID Dot Product, hwcap: 0x%x \n", hwcap);
      support_optimize_ops = true;
    } else {
      printf("Hw cap NOT support SIMD Dot Product, hwcap: 0x%x\n", hwcap);
    }
#endif
#endif
    if (!support_optimize_ops) {
      return;
    }
    optimized_op_handler_ = dlopen(OPTIMIZE_SHARED_LIBRARY_PATH, RTLD_LAZY);
    if (optimized_op_handler_ == nullptr) {
      printf("Open optimize shared library failed.\n");
    }
  }

  ~OptimizeModule() = default;

  static OptimizeModule *GetInstance() {
    static OptimizeModule opt_module;
    return &opt_module;
  }
  void *optimized_op_handler_ = nullptr;
};

#endif  // MINDSPORE_LITE_SRC_RUNTIME_KERNEL_ARM_OPCLIB_OPTIMIZED_KERNEL_H_

