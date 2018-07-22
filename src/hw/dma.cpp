#include <mbed.h>

#include "debug/macros.h"

#include "dma.h"

namespace app::hw {

static uint32_t zero_words[] = {0, 0, 0, 0};

CopyDMA::CopyDMA() {
}

int CopyDMA::Init() {
  __HAL_RCC_DMA2_CLK_ENABLE();

  handle.Instance = DMA2_Stream0;       // Only DMA2 can do memory-to-memory
  handle.Init.Channel = DMA_CHANNEL_0;  // Can use only 1 chan per stream
  handle.Init.Direction = DMA_MEMORY_TO_MEMORY;
  handle.Init.PeriphInc = DMA_PINC_ENABLE;  // Source
  handle.Init.MemInc = DMA_MINC_ENABLE;     // Destination
  handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
  handle.Init.MemDataAlignment = DMA_PDATAALIGN_WORD;
  handle.Init.Mode = DMA_NORMAL;
  handle.Init.Priority = DMA_PRIORITY_LOW;
  handle.Init.MemBurst = DMA_MBURST_SINGLE;  // See AN4031 for burst reqs
  handle.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
  handle.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
  handle.Init.PeriphBurst = DMA_MBURST_SINGLE;

  if (HAL_OK != HAL_DMA_Init(&handle)) {
    return 1;
  }

  return 0;
}

int CopyDMA::CopyWordsUnsafe(
    uint32_t src_addr, uint32_t dst_addr, uint32_t num_words) {
  // DMA can process only up to 0xFFFF words.
  // Our DMA bursts have 4 word size.
  // (=> num_words must be multiple of 4!)
  // Our LTDC bursts have 64 byte size.
  // LTDC bursts can't cross the 1kB boundary.
  // (=> src_addr/dst_addr should be 64 byte = 16 word multiple!).
  // Highest multiple of 4 and 64 less than 0xFFFF is 0xFFC0.
  const uint32_t max_num_words_per_batch = 0xFFC0;

  // Batches of 0xFFC0 words.
  while (num_words > max_num_words_per_batch) {
    CopyMax65kWordsUnsafe(src_addr, dst_addr, max_num_words_per_batch);
    num_words -= max_num_words_per_batch;
    src_addr += sizeof(uint32_t) * max_num_words_per_batch;
    dst_addr += sizeof(uint32_t) * max_num_words_per_batch;
  }

  // Any remaining words (at most 0xFFC0 words).
  if (num_words > 0) {
    CopyMax65kWordsUnsafe(src_addr, dst_addr, num_words);
  }

  return 0;
}

int CopyDMA::CopyMax65kWordsUnsafe(
    uint32_t src_addr, uint32_t dst_addr, uint32_t num_words) {
  if (HAL_OK != HAL_DMA_Start(&handle, src_addr, dst_addr, num_words)) {
    return 1;
  }

  if (HAL_OK !=
      HAL_DMA_PollForTransfer(&handle, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY)) {
    return 1;
  }

  return 0;
}

ZeroDMA::ZeroDMA() {
}

int ZeroDMA::Init() {
  __HAL_RCC_DMA2_CLK_ENABLE();

  handle.Instance = DMA2_Stream0;       // Only DMA2 can do memory-to-memory
  handle.Init.Channel = DMA_CHANNEL_1;  // Can use only 1 chan per stream
  handle.Init.Direction = DMA_MEMORY_TO_MEMORY;
  handle.Init.PeriphInc = DMA_PINC_DISABLE;  // Source
  handle.Init.MemInc = DMA_MINC_ENABLE;      // Destination
  handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
  handle.Init.MemDataAlignment = DMA_PDATAALIGN_WORD;
  handle.Init.Mode = DMA_NORMAL;
  handle.Init.Priority = DMA_PRIORITY_LOW;
  handle.Init.MemBurst = DMA_MBURST_SINGLE;  // See AN4031 for burst reqs
  handle.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
  handle.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
  handle.Init.PeriphBurst = DMA_MBURST_SINGLE;

  if (HAL_OK != HAL_DMA_Init(&handle)) {
    return 1;
  }

  return 0;
}

int ZeroDMA::ZeroWordsUnsafe(uint32_t dst_addr, uint32_t num_words) {
  // DMA can process only up to 0xFFFF words.
  // Our DMA bursts have 4 word size.
  // (=> num_words MUST be multiple of 4!)
  // Our LTDC bursts have 64 byte size.
  // LTDC bursts can't cross the 1kB boundary.
  // (=> dst_addr SHOULD be 64 byte (16 word) multiple!).
  // Highest multiple 64 (and 4) less than 0xFFFF is 0xFFC0.
  const uint32_t max_num_words_per_batch = 0xFFC0;

  // Batches of 0xFFC0 words.
  while (num_words > max_num_words_per_batch) {
    ZeroMax65kWordsUnsafe(dst_addr, max_num_words_per_batch);
    num_words -= max_num_words_per_batch;
    dst_addr += sizeof(uint32_t) * max_num_words_per_batch;
  }

  // Any remaining words (at most 0xFFC0 words).
  if (num_words > 0) {
    ZeroMax65kWordsUnsafe(dst_addr, num_words);
  }

  return 0;
}

int ZeroDMA::ZeroMax65kWordsUnsafe(uint32_t dst_addr, uint32_t num_words) {
  if (HAL_OK !=
      HAL_DMA_Start(&handle, (uint32_t)zero_words, dst_addr, num_words)) {
    return 1;
  }

  if (HAL_OK !=
      HAL_DMA_PollForTransfer(&handle, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY)) {
    return 1;
  }

  return 0;
}

}  // namespace app::hw
