#include "waterfall.h"

namespace app::ui {

Waterfall::Waterfall(
    app::hw::VolatileBuffer<uint8_t> &buffer,
    app::hw::CopyDMA &copy_dma,
    unsigned int size_x,
    unsigned int size_y)
    : buffer(buffer), copy_dma(copy_dma), size_x(size_x), size_y(size_y) {
}

int Waterfall::Render(app::hw::VolatileBuffer<uint8_t> &output) {
  if (0 != CopyLines(output, 0, size_y - line, line)) {
    return 1;
  }
  if (0 != CopyLines(output, line, 0, size_y - line)) {
    return 1;
  }
  return 0;
}

int Waterfall::CopyLines(
    app::hw::VolatileBuffer<uint8_t> &output,
    int src_line,
    int dst_line,
    int num_lines) {
  if (num_lines <= 0) return 0;

  uint32_t src_buf_addr = (uint32_t)buffer.Data();
  uint32_t dst_buf_addr = (uint32_t)output.Data();
  uint32_t src_offset = src_line * size_x;
  uint32_t dst_offset = dst_line * size_x;
  uint32_t src_addr = src_buf_addr + src_offset;
  uint32_t dst_addr = dst_buf_addr + dst_offset;
  uint32_t num_words = num_lines * size_x / sizeof(uint32_t);

  if (0 != copy_dma.CopyWordsUnsafe(src_addr, dst_addr, num_words)) {
    return 1;
  }

  return 0;
}

void Waterfall::Shift() {
  if (line <= 0) {
    line = size_y;
  }
  line--;
}

}  // namespace app::ui
