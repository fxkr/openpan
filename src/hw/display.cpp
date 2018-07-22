#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include <mbed.h>

#include "Drivers/BSP/STM32746G-Discovery/stm32746g_discovery_audio.h"
#include "Drivers/BSP/STM32746G-Discovery/stm32746g_discovery_lcd.h"
#include "Drivers/BSP/STM32746G-Discovery/stm32746g_discovery_ts.h"

#include "data/gradient.h"
#include "debug/counter.h"
#include "debug/macros.h"
#include "hw/dma.h"
#include "hw/volatile_buffer.h"
#include "hw/volatile_triple_buffer.h"

#include "hw/display.h"

// Defined by mbed
extern LTDC_HandleTypeDef hLtdcHandler;

namespace app::hw {

// Overridden to use slower LCD clock to fix flickering due to AHB contention.
void BSP_LCD_ClockConfig(LTDC_HandleTypeDef *hltdc, void *Params) {
  static RCC_PeriphCLKInitTypeDef periph_clk_init_struct;

  // RK043FN48H LCD clock configuration
  // PLLSAI_VCO Input = HSE_VALUE/PLL_M = 1 Mhz
  // PLLSAI_VCO Output = PLLSAI_VCO Input * PLLSAIN = 192 Mhz
  // PLLLCDCLK = PLLSAI_VCO Output/PLLSAIR = 192/7 = 27.4 Mhz
  // LTDC clock frequency = PLLLCDCLK / LTDC_PLLSAI_DIVR_4 = 27.4/4 = 6.85Mhz
  periph_clk_init_struct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
  periph_clk_init_struct.PLLSAI.PLLSAIN = 192;
  periph_clk_init_struct.PLLSAI.PLLSAIR = 7;  // Default 5, range 2-7
  periph_clk_init_struct.PLLSAIDivR = RCC_PLLSAIDIVR_4;
  HAL_RCCEx_PeriphCLKConfig(&periph_clk_init_struct);
}

Display::Display(
    app::debug::Debug &dbg,
    VolatileTripleBuffer<uint8_t> &layer0,
    VolatileTripleBuffer<uint32_t> &layer1,
    CopyDMA &copy_dma,
    app::debug::Counter &ltdc_underrun_counter)
    : dbg(dbg),
      layer0(layer0),
      layer1(layer1),
      copy_dma(copy_dma),
      ltdc_underrun_counter(ltdc_underrun_counter) {}

int Display::Init() {
  crash_if_not(dbg, layer0.size == size_x * size_y * sizeof(uint32_t));
  crash_if_not(dbg, layer1.size == size_x * size_y * sizeof(uint32_t));

  if (LCD_OK != BSP_LCD_Init()) {
    return 1;
  }

  crash_if_not(dbg, size_x == BSP_LCD_GetXSize());
  crash_if_not(dbg, size_y == BSP_LCD_GetYSize());

  // Init layer 0 (background)
  LCD_LayerCfgTypeDef layer_cfg;
  layer_cfg.WindowX0 = 0;
  layer_cfg.WindowX1 = BSP_LCD_GetXSize();
  layer_cfg.WindowY0 = 0;
  layer_cfg.WindowY1 = BSP_LCD_GetYSize();
  layer_cfg.PixelFormat = LTDC_PIXEL_FORMAT_L8;
  layer_cfg.FBStartAdress = layer0.GetFrontBuffer().addr;
  layer_cfg.Alpha = 255;
  layer_cfg.Alpha0 = 0;
  layer_cfg.Backcolor.Blue = 0;
  layer_cfg.Backcolor.Green = 0;
  layer_cfg.Backcolor.Red = 0;
  layer_cfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  layer_cfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  layer_cfg.ImageWidth = BSP_LCD_GetXSize();
  layer_cfg.ImageHeight = BSP_LCD_GetYSize();
  HAL_LTDC_ConfigLayer(&hLtdcHandler, &layer_cfg, 0);

  // Init layer 1 (foreground)
  layer_cfg.WindowX0 = 0;
  layer_cfg.WindowX1 = BSP_LCD_GetXSize();
  layer_cfg.WindowY0 = 0;
  layer_cfg.WindowY1 = BSP_LCD_GetYSize();
  layer_cfg.PixelFormat = LTDC_PIXEL_FORMAT_ARGB8888;
  layer_cfg.FBStartAdress = layer1.GetFrontBuffer().addr;
  layer_cfg.Alpha = 255;
  layer_cfg.Alpha0 = 0;
  layer_cfg.Backcolor.Blue = 0;
  layer_cfg.Backcolor.Green = 0;
  layer_cfg.Backcolor.Red = 0;
  layer_cfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  layer_cfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  layer_cfg.ImageWidth = BSP_LCD_GetXSize();
  layer_cfg.ImageHeight = BSP_LCD_GetYSize();
  HAL_LTDC_ConfigLayer(&hLtdcHandler, &layer_cfg, 1);

  // Configure initial framebuffers
  BSP_LCD_DisplayOn();
  HAL_LTDC_SetAddress_NoReload(&hLtdcHandler, layer0.GetFrontBuffer().addr, 0);
  HAL_LTDC_SetAddress_NoReload(&hLtdcHandler, layer1.GetFrontBuffer().addr, 1);
  HAL_LTDC_Reload(&hLtdcHandler, LTDC_RELOAD_IMMEDIATE);

  if (HAL_LTDC_ConfigCLUT(&hLtdcHandler, app::data::GRADIENT, 256, 0) !=
      HAL_OK) {
    return 1;
  }
  if (HAL_LTDC_EnableCLUT(&hLtdcHandler, 0) != HAL_OK) {
    return 1;
  }

  // LTDC interrupts needed for buffer flipping
  HAL_NVIC_SetPriority(LTDC_IRQn, 0xE, 0);
  HAL_NVIC_EnableIRQ(LTDC_IRQn);

  __HAL_LTDC_ENABLE_IT(&hLtdcHandler, LTDC_IT_FU);

  return 0;
}

void Display::Flip() {
  // Ensure Register Reload interrupt is disabled
  // (Mostly needed when a frambuffer switch is pending already)
  __HAL_LTDC_DISABLE_IT(&hLtdcHandler, LTDC_IT_RR);

  layer0.FlipBackBuffer();
  layer1.FlipBackBuffer();

  // Request swap of front buffer and next front buffer by ISR
  switch_front_buffer = true;
  HAL_LTDC_SetAddress_NoReload(
      &hLtdcHandler, layer0.GetNextFrontBuffer().addr, 0);
  HAL_LTDC_SetAddress_NoReload(
      &hLtdcHandler, layer1.GetNextFrontBuffer().addr, 1);
  HAL_LTDC_Reload(&hLtdcHandler, LTDC_RELOAD_VERTICAL_BLANKING);

  // Reenable interrupt to switch buffers at next VSYNC
  __HAL_LTDC_ENABLE_IT(&hLtdcHandler, LTDC_IT_RR);
}

void Display::HandleReload() {
  if (!switch_front_buffer) {
    return;
  }
  switch_front_buffer = false;

  layer0.FlipFrontBuffer();
  layer1.FlipFrontBuffer();

  // Don't call us again unless another buffer switch is needed
  __HAL_LTDC_DISABLE_IT(&hLtdcHandler, LTDC_IT_RR);
}

VolatileBuffer<uint32_t> &Display::GetForeground() {
  return layer1.GetBackBuffer();
}

VolatileBuffer<uint8_t> &Display::GetBackground() {
  return layer0.GetBackBuffer();
}

void Display::HandleUnderrun() { ltdc_underrun_counter.Increment(); }

void Display::HandleLtdcIRQ() { HAL_LTDC_IRQHandler(&hLtdcHandler); }

}  // namespace app::hw
