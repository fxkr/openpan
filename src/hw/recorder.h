#pragma once

#include <stdint.h>

#include <arm_math.h>

#include "debug/counter.h"
#include "hw/volatile_buffer.h"
#include "structs/complex.h"

namespace app::hw {

static const int recorder_num_samples = 512;

class Recorder {
 private:
  app::debug::Debug &dbg;

  VolatileBuffer<app::structs::Complex<int16_t>> &buffer;
  VolatileBuffer<app::structs::Complex<int16_t>> lower_half_buffer;
  VolatileBuffer<app::structs::Complex<int16_t>> upper_half_buffer;

  uint32_t dma_state = 0;

  const int dma_double_buffer_num_halfwords = buffer.size / sizeof(int16_t);

  app::structs::Complex<float32_t> sig_buffer[recorder_num_samples];

  app::debug::Counter &missed_audio_counter;
  app::debug::Counter &late_audio_read_counter;

 public:
  const int num_samples = recorder_num_samples;

  Recorder(app::debug::Debug &dbg,
           VolatileBuffer<app::structs::Complex<int16_t>> &audio_buf,
           app::debug::Counter &missed_audio_counter,
           app::debug::Counter &late_audio_read_counter);

  int Init();

  app::structs::Complex<float32_t> *Tick();

  void HandleAudioInError();
  void HandleHalfTransferComplete();
  void HandleTransferComplete();
};

}  // namespace app::hw
