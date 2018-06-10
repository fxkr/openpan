#pragma once

#include <stdint.h>

#include <mbed.h>

#include "cmsis/TARGET_CORTEX_M/core_cm7.h"

namespace app::hw {

class PerfTimer {
 public:
  PerfTimer();
  inline __attribute__((always_inline)) void Reset();
  inline __attribute__((always_inline)) uint32_t GetCycles();
};

inline __attribute__((always_inline)) void PerfTimer::Reset() {
  DWT->CYCCNT = 0;
}

inline __attribute__((always_inline)) uint32_t PerfTimer::GetCycles() {
  return DWT->CYCCNT;
}

}  // namespace app::hw
