#pragma once

#include <stdint.h>

#include "hw/dma.h"
#include "hw/volatile_buffer.h"

namespace app::ui {

class Waterfall {
 private:
  unsigned int size_x;
  unsigned int size_y;

  unsigned int line = 0;

  app::hw::CopyDMA &copy_dma;
  app::hw::VolatileBuffer<uint8_t> buffer;

  int CopyLines(app::hw::VolatileBuffer<uint8_t> &output,
                int src_line,
                int dst_line,
                int num_lines);

 public:
  Waterfall(app::hw::VolatileBuffer<uint8_t> &buffer,
            app::hw::CopyDMA &copy_dma,
            unsigned int size_x,
            unsigned int size_y);

  void Shift();
  inline void Set(unsigned int x, uint8_t color);

  int Render(app::hw::VolatileBuffer<uint8_t> &output);
};

inline void Waterfall::Set(unsigned int i, uint8_t color) {
  buffer.Data()[line * size_x + i] = color;
}

}  // namespace app::ui
