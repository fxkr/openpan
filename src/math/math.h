#pragma once

#include <stdint.h>

#include <arm_math.h>

namespace app::math {

// Fast log2 approximation.
//
// Input is a normalized, single-precision IEEE754 float
// greater than the basis (2.0).
//
// Tested: At inputs around 2^30, error better than 0.05% of output.
// Execution time around 25-55 cycles (expect 45 cycles) on STM32F746G.
// 25x speedup over standard library log2, which is around 1250 cycles.
inline float32_t fast_log2(float32_t val) {
  if (val < 2.0) {
    return log2(val);  // for correctness
  }

  union {
    float32_t f;
    uint32_t x;
  } u = {val};

  const uint32_t int_log2 = ((u.x >> 23) & 255) - 128;
  u.x &= ~(255 << 23);
  u.x += 127 << 23;
  u.f = ((-1.0f / 3) * u.f + 2) * u.f - 2.0f / 3;  // optional; for 1/10 error
  return int_log2 + u.f;
}

// Returns a value clipped to a lower and upper bound.
template <typename T, T min, T max>
inline T limit(T x) {
  static_assert(min <= max);
  if (x < min) {
    return min;
  } else if (x > max) {
    return max;
  } else {
    return x;
  }
}

}  // namespace app::math
