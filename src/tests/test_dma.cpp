#include <mbed.h>

#include "Drivers/BSP/STM32746G-Discovery/stm32746g_discovery_lcd.h"

#include "application.h"
#include "debug/class.h"
#include "debug/macros.h"
#include "hw/dma.h"

const uint32_t dma_test_area_words = 0x24680;

// Testing area pointers
//
// Includes canaries to ensure no write before/after.
// Size is multiple of 4 words to accomodate DMA burst size.
// Size is larger than 0xFFFF to test split into multiple batches.
struct dma_test_area {
  uint32_t canary_before[4];
  uint32_t data_area[dma_test_area_words];
  uint32_t canary_after[4];
};

void test_copy_dma(app::debug::Debug& dbg) {
  dbg.printf("- %s\n", __func__);

  struct dma_test_area* src_test_area =
      (struct dma_test_area*)LCD_FB_START_ADDRESS;
  struct dma_test_area* dst_test_area =
      (struct dma_test_area*)(LCD_FB_START_ADDRESS + 0x400000);

  uint32_t src_area_addr = (uint32_t)src_test_area->data_area;
  uint32_t dst_area_addr = (uint32_t)dst_test_area->data_area;

  // Ensure test is sane
  crash_if_not(dbg, dma_test_area_words % 4 == 0);  // DMA burst size
  crash_if_not(dbg, dma_test_area_words > 0xFFFF);  // Multiple batches

  // Initialize memory
  for (int i = 0; i < 3; i++) {
    src_test_area->canary_before[i] = 0x10;
    src_test_area->canary_after[i] = 0x20;
    dst_test_area->canary_before[i] = 0x30;
    dst_test_area->canary_after[i] = 0x40;
  }
  for (uint32_t i = 0; i < dma_test_area_words; i++) {
    src_test_area->data_area[i] = i;
    dst_test_area->data_area[i] = 0xAA + i % 2;
  }

  // Zero it
  app::hw::CopyDMA copy_dma;
  crash_if(dbg, 0 != copy_dma.Init());
  crash_if(
      dbg,
      0 != copy_dma.CopyWordsUnsafe(
               src_area_addr, dst_area_addr, dma_test_area_words));

  // Assert everything within bounds is copied, everything outside is untouched
  for (uint32_t i = 0; i < dma_test_area_words; i++) {
    crash_if(dbg, src_test_area->data_area[i] != i);
    crash_if(dbg, dst_test_area->data_area[i] != i);
  }
  for (int i = 0; i < 3; i++) {
    crash_if(dbg, src_test_area->canary_before[i] != 0x10);
    crash_if(dbg, src_test_area->canary_after[i] != 0x20);
    crash_if(dbg, dst_test_area->canary_before[i] != 0x30);
    crash_if(dbg, dst_test_area->canary_after[i] != 0x40);
  }
}

void test_zero_dma(app::debug::Debug& dbg) {
  dbg.printf("- %s\n", __func__);

  struct dma_test_area* test_area = (struct dma_test_area*)LCD_FB_START_ADDRESS;
  uint32_t zero_area_addr = (uint32_t)test_area->data_area;

  // Ensure test is sane
  crash_if_not(dbg, dma_test_area_words % 4 == 0);  // DMA burst size
  crash_if_not(dbg, dma_test_area_words > 0xFFFF);  // Multiple batches

  // Initialize memory
  for (int i = 0; i < 3; i++) {
    test_area->canary_before[i] = 0x10;
    test_area->canary_after[i] = 0x90;
  }
  for (uint32_t i = 0; i < dma_test_area_words; i++) {
    test_area->data_area[i] = i;
  }

  // Zero it
  app::hw::ZeroDMA zero_dma;
  crash_if(dbg, 0 != zero_dma.Init());
  crash_if(
      dbg, 0 != zero_dma.ZeroWordsUnsafe(zero_area_addr, dma_test_area_words));

  // Assert everything within bounds is zeroed, everything outside is not
  for (uint32_t i = 0; i < dma_test_area_words; i++) {
    crash_if(dbg, test_area->data_area[i] != 0x00);
  }
  for (int i = 0; i < 3; i++) {
    crash_if(dbg, test_area->canary_before[i] != 0x10);
    crash_if(dbg, test_area->canary_after[i] != 0x90);
  }
}

void test_dma(app::debug::Debug& debug) {
  test_zero_dma(debug);
  test_copy_dma(debug);
}
