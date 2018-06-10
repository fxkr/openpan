#include <mbed.h>

#include "cmsis/TARGET_CORTEX_M/core_cm7.h"

#include "perf_timer.h"

namespace app::hw {

PerfTimer::PerfTimer() {
  CoreDebug->DEMCR = CoreDebug_DEMCR_TRCENA_Msk;
  __DSB();
  DWT->LAR = 0xC5ACCE55;
  __DSB();
  DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL = DWT_CTRL_CYCCNTENA_Msk;
}

}  // namespace app::hw
