#pragma once

#include "hw/display.h"
#include "hw/volatile_buffer.h"

namespace app::ui {

class Painter {
 private:
  unsigned int size_x;
  unsigned int size_y;

  app::hw::VolatileBuffer<uint32_t> *buffer = nullptr;
  volatile uint32_t *buffer_data = nullptr;

 public:
  Painter(unsigned int size_x, unsigned int size_y);
  void SetBuffer(app::hw::VolatileBuffer<uint32_t> &buffer);

  inline void DrawPixel(int x, int y, uint32_t color);
  void DrawText(int x, int y, uint32_t fg, uint32_t bg, const char *text);
  void DrawChar(int x0, int y0, uint32_t fg, uint32_t bg, const char c);
  inline unsigned int SizeX();
  inline unsigned int SizeY();
};

inline void Painter::DrawPixel(int x, int y, uint32_t color) {
  buffer_data[y * size_x + x] = color;
}

inline unsigned int Painter::SizeX() { return size_x; }

inline unsigned int Painter::SizeY() { return size_y; }

}  // namespace app::ui
