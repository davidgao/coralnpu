/*
 * Copyright 2026 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>

int8_t op1[32] __attribute__((section(".data"))) __attribute__((aligned(16)));
int8_t op2[256] __attribute__((section(".data"))) __attribute__((aligned(16)));

void run_compiled() {
  // Throughput: 1 iteration every 87 cycles
  // ptr1 increments are omitted.
  const int8_t *ptr2 = &op2[0];
  asm volatile(
      "vsetivli zero, 0x10, e8, m1, ta, ma;"
      "vle8.v v12, %[ptr1];"
      "vsetivli zero, 0x1, e32, m1, ta, ma;"
      "vslidedown.vi v18, v9, 0x1;"
      "vsetivli zero, 0x10, e8, m1, ta, ma;"
      "vle8.v v13, (%[ptr2]);"
      "addi %[ptr2], %[ptr2], 16;"  // fake 1 row increment
      "vsetivli zero, 0x1, e32, m1, ta, ma;"
      "vslidedown.vi v19, v9, 0x2;"
      "vsetivli zero, 0x10, e8, m1, ta, ma;"
      "vle8.v v20, (%[ptr2]);"
      "addi %[ptr2], %[ptr2], 16;"  // fake 1 row increment
      "vsetivli zero, 0x1, e32, m1, ta, ma;"
      "vslidedown.vi v21, v9, 0x3;"
      "vsetivli zero, 0x10, e16, m2, ta, ma;"
      "vsext.vf2 v10, v12;"
      "vsext.vf2 v16, v13;"
      "vwmul.vv v12, v16, v10;"
      "vsetvli zero, zero, e32, m4, ta, ma;"
      "vredsum.vs v9, v12, v9;"
      "vle8.v  v22, (%[ptr2]);"
      "addi %[ptr2], %[ptr2], 16;"  // fake 1 row increment
      "vsetvli zero, zero, e16, m2, ta, ma;"
      "vsext.vf2 v16, v20;"
      "vwmul.vv v12, v16, v10;"
      "vsetvli zero, zero, e32, m4, ta, ma;"
      "vredsum.vs v18, v12, v18;"
      "vle8.v  v20, (%[ptr2]);"
      "addi %[ptr2], %[ptr2], 16;"  // fake 1 row increment
      "vsetvli zero, zero, e16, m2, ta, ma;"
      "vsext.vf2 v16, v22;"
      "vwmul.vv v12, v16, v10;"
      "vsetvli zero, zero, e32, m4, ta, ma;"
      "vredsum.vs v19, v12, v19;"
      "vsetvli zero, zero, e16, m2, ta, ma;"
      "vsext.vf2 v16, v20;"
      "vwmul.vv v12, v16, v10;"  // 37
      "vsetvli zero, zero, e32, m4, ta, ma;"
      "vredsum.vs v10, v12, v21;"
      "vsetivli zero, 0x4, e32, m1, tu, ma;"
      "vslideup.vi v19, v10, 0x1;"
      "vslideup.vi v18, v19, 0x1;"
      "vslideup.vi v9, v18, 0x1;"
      : [ptr2] "+r"(ptr2)
      : [ptr1] "A"(op1), "m"(op2)
      : "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21",
        "v22", "vl", "vtype");
}

void run_vx_orig() {
  // Throughput: 1 iteration every 76 cycles
  uint32_t op1_bundle = ((volatile uint32_t *)op1)[0];
  const int8_t *ptr2  = &op2[0];
  asm volatile(
      "vsetvli zero, %[vl], e8, m4, ta, ma;"
      "vle8.v v4, (%[ptr2]);"
      "addi %[ptr2], %[ptr2], 64;"  // fake 1 row increment
      "vwmul.vx v8, v4, %[op1];"
      "vsetvli zero, %[vl_f2], e16, m4, ta, ma;"
      "vwadd.wv v16, v16, v8;"
      "vwadd.wv v24, v24, v12;"
      "srli %[op1], %[op1], 8;"

      "vsetvli zero, %[vl], e8, m4, ta, ma;"
      "vle8.v v4, (%[ptr2]);"
      "addi %[ptr2], %[ptr2], 64;"  // fake 1 row increment
      "vwmul.vx v8, v4, %[op1];"
      "vsetvli zero, %[vl_f2], e16, m4, ta, ma;"
      "vwadd.wv v16, v16, v8;"
      "vwadd.wv v24, v24, v12;"
      "srli %[op1], %[op1], 8;"

      "vsetvli zero, %[vl], e8, m4, ta, ma;"
      "vle8.v v4, (%[ptr2]);"
      "addi %[ptr2], %[ptr2], 64;"  // fake 1 row increment
      "vwmul.vx v8, v4, %[op1];"
      "vsetvli zero, %[vl_f2], e16, m4, ta, ma;"
      "vwadd.wv v16, v16, v8;"
      "vwadd.wv v24, v24, v12;"
      "srli %[op1], %[op1], 8;"

      "vsetvli zero, %[vl], e8, m4, ta, ma;"
      "vle8.v v4, (%[ptr2]);"
      "vwmul.vx v8, v4, %[op1];"
      "vsetvli zero, %[vl_f2], e16, m4, ta, ma;"
      "vwadd.wv v16, v16, v8;"
      "vwadd.wv v24, v24, v12;"

      : [op1] "+r"(op1_bundle), [ptr2] "+r"(ptr2)
      : [vl] "r"(64), [vl_f2] "r"(32), "m"(op2)
      : "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17",
        "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27", "v28", "v29", "v30",
        "v31", "vl", "vtype");
}

void run_vx_pair() {
  // Throughput: 1 iteration every 60 cycles
  uint32_t op1_bundle = ((volatile uint32_t *)op1)[0];
  const int8_t *ptr2  = &op2[0];
  asm volatile(
      "vsetvli zero, %[vl], e8, m4, ta, ma;"
      "vle8.v v4, (%[ptr2]);"
      "addi %[ptr2], %[ptr2], 64;"  // fake 1 row increment
      "vwmul.vx v8, v4, %[op1];"
      "srli %[op1], %[op1], 8;"

      "vle8.v v4, (%[ptr2]);"
      "addi %[ptr2], %[ptr2], 64;"  // fake 1 row increment
      "vwmacc.vx v8, %[op1], v4;"
      "vsetvli zero, %[vl_f2], e16, m4, ta, ma;"
      "vwadd.wv v16, v16, v8;"
      "vwadd.wv v24, v24, v12;"
      "srli %[op1], %[op1], 8;"

      "vsetvli zero, %[vl], e8, m4, ta, ma;"
      "vle8.v v4, (%[ptr2]);"
      "addi %[ptr2], %[ptr2], 64;"  // fake 1 row increment
      "vwmul.vx v8, v4, %[op1];"
      "srli %[op1], %[op1], 8;"

      "vle8.v v4, (%[ptr2]);"
      "vwmacc.vx v8, %[op1], v4;"
      "vsetvli zero, %[vl_f2], e16, m4, ta, ma;"
      "vwadd.wv v16, v16, v8;"
      "vwadd.wv v24, v24, v12;"

      : [op1] "+r"(op1_bundle), [ptr2] "+r"(ptr2)
      : [vl] "r"(64), [vl_f2] "r"(32), "m"(op2)
      : "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17",
        "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27", "v28", "v29", "v30",
        "v31", "vl", "vtype");
}

int main(void) {
  for (int i = 0; i < 10; ++i) {
    // run_compiled();
    // run_vx_orig();
    run_vx_pair();
  }
  return 0;
}