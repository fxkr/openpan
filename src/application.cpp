#include <arm_const_structs.h>
#include <arm_math.h>

#include <mbed.h>

#include "Drivers/BSP/STM32746G-Discovery/stm32746g_discovery_lcd.h"

#include "data/gradient.h"
#include "debug/class.h"
#include "debug/counter.h"
#include "debug/macros.h"
#include "hw/volatile_buffer.h"
#include "hw/volatile_triple_buffer.h"
#include "math/math.h"
#include "ui/canvas.h"

#include "application.h"

namespace app {

// Constants for offsetting KX3's 8kHz Freq Shift. Derivation:
//
// * Need 8kHz * 48kHz / 512 bins = 85 bins shift (from center).
// * Not all FFT bins can be displayed (fft size > display width).
//   Therefore, shift receive bin towards center of display.
//   We can shift up to 512/2 - 480/2 = 16 bins.
// * Receive bin is still off-center. Therefore shift scale towards receive bin.
//
// Where:
// * 1 column represents 1 bin.
// * Sampling frequency is 48 kHz.
// * Receive frequency is 8 kHz from center frequency.
// * FFT size is 512 (complex inputs/outputs).
// * 480 columns / bins are visible.
static unsigned int total_shift = 85;  // 8kHz/48kHz / 512 samples, result in px
static unsigned int waterfall_shift = 16;  // Can only shift within 512 samples
static unsigned int ui_shift = total_shift - waterfall_shift;

Application::Application(app::debug::Debug &dbg,
                         app::hw::Display &display,
                         app::ui::Painter &canvas,
                         app::hw::Recorder &recorder,
                         app::ui::Waterfall &waterfall)
    : dbg(dbg),
      display(display),
      canvas(canvas),
      recorder(recorder),
      waterfall(waterfall) {}

int Application::Init() {
  render_job_ready = true;
  return 0;
}

void Application::Loop() {
  if (processing_job_ready) {
    processing_job_ready = false;
    ProcessingJob();
  }
  if (render_job_ready) {
    render_job_ready = false;
    RenderJob();
  }
}

void Application::ProcessingJob() {
  processing_job_ready = true;

  app::structs::Complex<float32_t> *sig_buffer = recorder.Tick();
  if (!sig_buffer) {
    return;
  }

  const arm_cfft_instance_f32 *fft_instance = &arm_cfft_sR_f32_len512;
  crash_if(dbg, fft_instance->fftLen != recorder.num_samples);
  arm_cfft_f32(fft_instance, (float32_t *)sig_buffer, 0, 1);

  waterfall.Shift();

  for (unsigned int i = 0; i < 480; i++) {
    // Convert column to bin
    unsigned int bin = i;  // Note: all intermediate values must be non-negative
    bin = 480 - bin;       // Cancel FFT's frequency inversion
    bin += 512 - 240;      // Shift zero bin to center column
    bin -= waterfall_shift;  // Cancel KX3's 8kHz shift
    bin %= 512;              // Get array index

    // Convert to power
    float32_t real = sig_buffer[bin].real;
    float32_t imag = sig_buffer[bin].imag;
    float32_t mag_unscaled_squared = real * real + imag * imag;
    float32_t power = app::math::fast_log2(mag_unscaled_squared);

    // Average
    float32_t alpha = 0.33;
    float32_t avg_power = powers[i] = power * alpha + powers[i] * (1.0 - alpha);

    // Offset and scale
    float32_t disp_power = (avg_power - 28) * 22;

    // Store
    uint8_t color = app::math::limit<int32_t, 0, 255>(disp_power);
    waterfall.Set(i, color);
  }

  render_job_ready = true;
}

void Application::RenderJob() {
  app::ui::Painter &p = canvas;

  // Background
  waterfall.Render(display.GetBackground());

  // Foreground
  canvas.SetBuffer(display.GetForeground());

  // Foreground: grid
  const uint32_t grid_color = 0xFF333333;
  for (unsigned int y = 0; y < p.SizeY(); y++) {
    if (y % 6 < 3) {
      continue;
    }
    p.DrawPixel(27 + ui_shift, y, grid_color);   // -20kHz
    p.DrawPixel(80 + ui_shift, y, grid_color);   // -15kHz
    p.DrawPixel(133 + ui_shift, y, grid_color);  // -10kHz
    p.DrawPixel(187 + ui_shift, y, grid_color);  // -5kHz
    p.DrawPixel(239 + ui_shift, y, grid_color);  // Center
    p.DrawPixel(240 + ui_shift, y, grid_color);  // Center
    p.DrawPixel(293 + ui_shift, y, grid_color);  // +5kHz
    p.DrawPixel(347 + ui_shift, y, grid_color);  // +10kHz
  }

  // Foreground: color key
  for (unsigned int x = 0; x < 8; x++) {
    p.DrawPixel(x, 0, 0xFFFFFFFF);
    for (unsigned int y = 0; y < 256; y++) {
      p.DrawPixel(x, y, app::data::GRADIENT[255 - y]);
    }
    for (unsigned int y = 256; y < p.SizeY(); y++) {
      p.DrawPixel(x, y, 0xFF000000);
    }
  }

  // Foreground: menu bar
  const uint32_t menu_bg_color = 0xFF000000;
  for (unsigned int x = 0; x < p.SizeX(); x++) {
    for (unsigned int y = p.SizeY() - 14; y < p.SizeY(); y++) {
      p.DrawPixel(x, y, 0xFF000000);
    }
  }

  // Foreground: menu bar, scale
  const uint32_t menu_text_color = 0xFFFFFFFF;
  p.DrawText(27 - 10 + ui_shift, 260, menu_text_color, menu_bg_color, "-20");
  p.DrawText(80 - 10 + ui_shift, 260, menu_text_color, menu_bg_color, "-15");
  p.DrawText(133 - 10 + ui_shift, 260, menu_text_color, menu_bg_color, "-10");
  p.DrawText(187 - 7 + ui_shift, 260, menu_text_color, menu_bg_color, "-5");
  p.DrawText(240 - 3 + ui_shift, 260, menu_text_color, menu_bg_color, "0");
  p.DrawText(293 - 7 + ui_shift, 260, menu_text_color, menu_bg_color, "+5");
  p.DrawText(347 - 10 + ui_shift, 260, menu_text_color, menu_bg_color, "+10");
  display.Flip();
}

void Application::HandleAudioInHalfTransferComplete() {
  recorder.HandleHalfTransferComplete();
  processing_job_ready = true;
}

void Application::HandleAudioInTransferComplete() {
  recorder.HandleTransferComplete();
  processing_job_ready = true;
}

void Application::HandleAudioInError() { recorder.HandleAudioInError(); }

void Application::HandleLtdcUnderrun() { display.HandleUnderrun(); }

void Application::HandleLtdcReload() { display.HandleReload(); }

void Application::HandleLtdcIRQ() { display.HandleLtdcIRQ(); }

}  // namespace app
