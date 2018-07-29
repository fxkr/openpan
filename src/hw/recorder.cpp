#include <stdbool.h>

#include <arm_math.h>

#include "Drivers/BSP/STM32746G-Discovery/stm32746g_discovery_audio.h"

#include "debug/counter.h"
#include "debug/macros.h"

#include "hw/volatile_buffer.h"

#include "recorder.h"

namespace app::hw {

static const uint32_t LOWER_HALF_READABLE_BIT = 1 << 0;
static const uint32_t UPPER_HALF_READABLE_BIT = 1 << 1;
static const uint32_t BOTH_HALF_READABLE_BITS =
    LOWER_HALF_READABLE_BIT | UPPER_HALF_READABLE_BIT;

extern "C" void EXTI15_10_IRQHandler(void);

Recorder::Recorder(
    app::debug::Debug &dbg,
    VolatileBuffer<app::structs::Complex<int16_t>> &audio_buf,
    app::debug::Counter &missed_audio_counter,
    app::debug::Counter &late_audio_read_counter)
    : dbg(dbg),
      buffer(audio_buf),
      lower_half_buffer(audio_buf.LowerHalf()),
      upper_half_buffer(audio_buf.UpperHalf()),
      missed_audio_counter(missed_audio_counter),
      late_audio_read_counter(late_audio_read_counter) {
}

int Recorder::Init() {
  // WM8994 spec, register 0210h (AIF1 Rate) / 0211h (AIF2 Rate):
  // "Note that 88.2kHz and 96kHz modes are supported
  // for AIF1 input (DAC playback) only." (Same for AIF2)
  // (It seems many people only read the first page of the spec.)
  if (AUDIO_OK != BSP_AUDIO_IN_InitEx(
                      INPUT_DEVICE_INPUT_LINE_1, AUDIO_FREQUENCY_48K, 0, 0)) {
    return 1;
  }

  if (AUDIO_OK !=
      BSP_AUDIO_IN_Record(
          (uint16_t *)buffer.addr, dma_double_buffer_num_halfwords)) {
    return 1;
  }

  // BSP_AUDIO_IN_MspInit overrides interrupt config.
  // Revert, because we're sharing it with the touchscreen and the button.
  // See AUDIO_IN_INT_IRQHandler for what handler needs to do for audio.
  IRQn_Type irqn = (IRQn_Type)(AUDIO_IN_INT_IRQ);
  NVIC_ClearPendingIRQ(irqn);
  NVIC_DisableIRQ(irqn);
  NVIC_SetPriority(irqn, AUDIO_IN_IRQ_PREPRIO);
  NVIC_SetVector(irqn, (uint32_t)EXTI15_10_IRQHandler);
  NVIC_EnableIRQ(irqn);

  return 0;
}

app::structs::Complex<float32_t> *Recorder::Read() {
  // Atomically read state and clear both bits
  uint32_t bit = __sync_fetch_and_and(&dma_state, ~BOTH_HALF_READABLE_BITS);

  // If one and only one of the bits is set, return corresponding buffer
  volatile app::structs::Complex<int16_t> *b;
  switch (bit) {
    case LOWER_HALF_READABLE_BIT:
      b = lower_half_buffer.Data();
      break;
    case UPPER_HALF_READABLE_BIT:
      b = upper_half_buffer.Data();
      break;
    default:
      return nullptr;  // Nothing available yet
  }

  // Copy data out of buffer (and do float conversion)
  for (int i = 0; i < num_samples; i++) {
    sig_buffer[i].real = b[i].real;
    sig_buffer[i].imag = b[i].imag;
  }

  // Clear bit to indicate comleted buffer read
  uint32_t state = __sync_and_and_fetch(&dma_state, ~bit);

  // Check for missed deadline (we may have been called too late)
  if (state != 0) {
    late_audio_read_counter.Increment();
    return nullptr;  // Data potentially corrupted, so discard
  }

  return sig_buffer;
}

void Recorder::HandleAudioInError() {
  crash(dbg);
}

void Recorder::HandleHalfTransferComplete() {
  if (READ_BIT(dma_state, UPPER_HALF_READABLE_BIT)) {
    missed_audio_counter.Increment();
  }
  CLEAR_BIT(dma_state, UPPER_HALF_READABLE_BIT);
  SET_BIT(dma_state, LOWER_HALF_READABLE_BIT);
}

void Recorder::HandleTransferComplete() {
  if (READ_BIT(dma_state, LOWER_HALF_READABLE_BIT)) {
    missed_audio_counter.Increment();
  }
  CLEAR_BIT(dma_state, LOWER_HALF_READABLE_BIT);
  SET_BIT(dma_state, UPPER_HALF_READABLE_BIT);
}

}  // namespace app::hw
