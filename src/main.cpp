#include <arm_const_structs.h>
#include <arm_math.h>

#include <mbed.h>

#include "Drivers/BSP/STM32746G-Discovery/stm32746g_discovery_audio.h"
#include "Drivers/BSP/STM32746G-Discovery/stm32746g_discovery_lcd.h"
#include "Drivers/BSP/STM32746G-Discovery/stm32746g_discovery_ts.h"

#include "application.h"
#include "debug/class.h"
#include "debug/counter.h"
#include "debug/funcs.h"
#include "debug/macros.h"
#include "hw/perf_timer.h"
#include "hw/volatile_buffer.h"

static const uint32_t fb_addr = LCD_FB_START_ADDRESS;
static const uint32_t fb_size = sizeof(uint32_t) * 272 * 480;

// Singletons called by interrupt handlers
static app::Application *volatile global_app = nullptr;
static app::debug::Debug *volatile global_dbg = nullptr;

int main() {
  HAL_Init();

  Serial serial(USBTX, USBRX);

  app::debug::Debug dbg(serial);
  app::debug::init(dbg);
  global_dbg = &dbg;

  dbg.printf("\nInit...\n");
  dbg.printf("Speed: %d Hz.\n", SystemCoreClock);

  BSP_SDRAM_Init();

  DigitalOut led(LED1);
  DigitalIn button(USER_BUTTON);

  volatile app::structs::Complex<int16_t> audio_buffer_alloc[2 * 512];

  app::debug::Counter ltdc_underrun_counter(dbg, "ltdc_underrun");
  app::debug::Counter missed_audio_counter(dbg, "missed_audio");
  app::debug::Counter late_audio_read_counter(dbg, "late_audio_read");
  app::hw::PerfTimer perf_timer;
  app::hw::CopyDMA copy_dma;
  app::hw::ZeroDMA zero_dma;
  app::hw::VolatileBuffer<uint8_t> buf0(
      dbg, zero_dma, fb_addr + 0 * fb_size, fb_size);
  app::hw::VolatileBuffer<uint8_t> buf1(
      dbg, zero_dma, fb_addr + 1 * fb_size, fb_size);
  app::hw::VolatileBuffer<uint8_t> buf2(
      dbg, zero_dma, fb_addr + 2 * fb_size, fb_size);
  app::hw::VolatileBuffer<uint32_t> buf3(
      dbg, zero_dma, fb_addr + 3 * fb_size, fb_size);
  app::hw::VolatileBuffer<uint32_t> buf4(
      dbg, zero_dma, fb_addr + 4 * fb_size, fb_size);
  app::hw::VolatileBuffer<uint32_t> buf5(
      dbg, zero_dma, fb_addr + 5 * fb_size, fb_size);
  app::hw::VolatileBuffer<uint8_t> wf_buf(
      dbg, zero_dma, fb_addr + 6 * fb_size, fb_size);
  app::hw::VolatileBuffer<app::structs::Complex<int16_t>> audio_buf(
      dbg, zero_dma, (uint32_t)&audio_buffer_alloc, sizeof(audio_buffer_alloc));
  app::hw::VolatileTripleBuffer<uint8_t> layer0(dbg, buf0, buf1, buf2);
  app::hw::VolatileTripleBuffer<uint32_t> layer1(dbg, buf3, buf4, buf5);
  app::ui::Waterfall waterfall(wf_buf, copy_dma, 480, 272);
  app::hw::Display display(
      dbg, layer0, layer1, copy_dma, ltdc_underrun_counter);
  app::hw::Recorder recorder(
      dbg, audio_buf, missed_audio_counter, late_audio_read_counter);
  app::ui::Painter canvas(480, 272);
  app::Application application(dbg, display, canvas, recorder, waterfall);

  crash_if(dbg, 0 != copy_dma.Init());
  crash_if(dbg, 0 != zero_dma.Init());
  crash_if(dbg, 0 != buf0.Init());
  crash_if(dbg, 0 != buf1.Init());
  crash_if(dbg, 0 != buf2.Init());
  crash_if(dbg, 0 != buf3.Init());
  crash_if(dbg, 0 != buf4.Init());
  crash_if(dbg, 0 != buf5.Init());
  crash_if(dbg, 0 != wf_buf.Init());
  crash_if(dbg, 0 != audio_buf.Init());
  crash_if(dbg, 0 != layer0.Init());
  crash_if(dbg, 0 != layer1.Init());
  crash_if(dbg, 0 != display.Init());
  crash_if(dbg, 0 != recorder.Init());
  crash_if(dbg, 0 != display.Init());
  crash_if(dbg, 0 != recorder.Init());
  crash_if(dbg, 0 != application.Init());

  dbg.printf("Init complete.\n");

  global_app = &application;
  while (true) {
    application.Loop();
  }
}

extern "C" void HardFault_Handler(void) {
  if (global_dbg) {
    app::debug::Debug &dbg = *global_dbg;
    crash(dbg);
  }
}

extern "C" void LTDC_IRQHandler(void) {
  if (global_app) {
    global_app->HandleLtdcIRQ();
  }
}

void HAL_LTDC_ReloadEventCallback(LTDC_HandleTypeDef *hltdc) {
  if (global_app) {
    global_app->HandleLtdcReload();
  }
}

void BSP_AUDIO_IN_Error_Callback(void) {
  if (global_app) {
    global_app->HandleAudioInError();
  }
}

void BSP_AUDIO_IN_HalfTransfer_CallBack(void) {
  if (global_app) {
    global_app->HandleAudioInHalfTransferComplete();
  }
}

void BSP_AUDIO_IN_TransferComplete_CallBack(void) {
  if (global_app) {
    global_app->HandleAudioInTransferComplete();
  }
}

void HAL_LTDC_ErrorCallback(LTDC_HandleTypeDef *hltdc) {
  if (HAL_LTDC_GetError(hltdc) & HAL_LTDC_ERROR_FU) {
    hltdc->ErrorCode &= ~HAL_LTDC_ERROR_FU;
    if (global_app) {
      global_app->HandleLtdcUnderrun();
    }
  }
}
