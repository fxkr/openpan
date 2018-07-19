#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include <mbed.h>

#include "Utilities/Fonts/font12.c"

#include "hw/display.h"

#include "canvas.h"

namespace app::ui {

Canvas::Canvas(unsigned int size_x, unsigned int size_y)
    : size_x(size_x), size_y(size_y) {}

void Canvas::SetBuffer(app::hw::VolatileBuffer<uint32_t> &new_buffer) {
  buffer = &new_buffer;
  buffer_data = new_buffer.Data();
}

void Canvas::DrawText(
    int x, int y, uint32_t fg, uint32_t bg, const char *text) {
  while (*text) {
    DrawChar(x, y, fg, bg, *text);
    x += Font12.Width;
    text++;
  }
}

void Canvas::DrawChar(int x0, int y0, uint32_t fg, uint32_t bg, const char c) {
  const uint8_t *bitmap_ptr = &Font12.table[(c - ' ') * Font12.Height];
  for (int y = 0; y < Font12.Height; y++) {
    uint8_t bitmap = *bitmap_ptr;
    for (int x = 0; x < Font12.Width; x++) {
      DrawPixel(x0 + x, y0 + y, bitmap & 0b1000000u ? fg : bg);
      bitmap <<= 1;
    }
    bitmap_ptr++;
  }
}

}  // namespace app::ui
