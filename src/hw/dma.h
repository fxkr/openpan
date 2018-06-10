#pragma once

#include <stdint.h>

#include <mbed.h>

namespace app::hw {

class CopyDMA {
 private:
  DMA_HandleTypeDef handle = {0};

  int CopyMax65kWordsUnsafe(uint32_t src_addr,
                            uint32_t dst_addr,
                            uint32_t num_words);

 public:
  CopyDMA();

  int Init();

  int CopyWordsUnsafe(uint32_t src_addr, uint32_t dst_addr, uint32_t num_words);
};

class ZeroDMA {
 private:
  DMA_HandleTypeDef handle = {0};

  int ZeroMax65kWordsUnsafe(uint32_t dst_addr, uint32_t num_words);

 public:
  ZeroDMA();

  int Init();

  int ZeroWordsUnsafe(uint32_t dst_addr, uint32_t num_words);
};

}  // namespace app::hw
