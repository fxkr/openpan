#pragma once

#include <stdint.h>

#include "debug/counter.h"
#include "hw/dma.h"
#include "hw/volatile_triple_buffer.h"

namespace app::hw {

class Display {
 private:
  app::debug::Debug &dbg;

  unsigned int size_x = 480;
  unsigned int size_y = 272;

  // Triple two-layer frame buffer (front is displayed, back is writable)
  VolatileTripleBuffer<uint8_t> &layer0;
  VolatileTripleBuffer<uint32_t> &layer1;

  // Signal for ISR to switch front buffer and next front buffers
  bool switch_front_buffer;

  CopyDMA &copy_dma;

  app::debug::Counter &ltdc_underrun_counter;

 public:
  Display(app::debug::Debug &dbg,
          VolatileTripleBuffer<uint8_t> &layer0,
          VolatileTripleBuffer<uint32_t> &layer1,
          CopyDMA &copy_dma,
          app::debug::Counter &ltdc_underrun_counter);

  int Init();

  void Flip();
  void HandleReload();
  void HandleUnderrun();

  int Blit(volatile uint8_t *src_buf, int src_line, int dst_line, int n_lines);
  int ScrolledBlit(volatile uint8_t *source, int first_line);
  int ScrolledBlit(volatile uint8_t *source, int first_line, int num_lines);

  VolatileBuffer<uint32_t> &GetForeground();
  VolatileBuffer<uint8_t> &GetBackground();

  void HandleLtdcIRQ();
};

}  // namespace app::hw
