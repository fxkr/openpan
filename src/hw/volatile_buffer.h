#pragma once

#include <stdint.h>

#include <mbed.h>

#include "debug/class.h"
#include "debug/macros.h"
#include "hw/dma.h"

namespace app::hw {

template <typename T>
class VolatileBuffer {
 private:
  app::debug::Debug &dbg;
  ZeroDMA &zero_dma;

 public:
  const uintptr_t addr;
  const uintptr_t size;

  VolatileBuffer(app::debug::Debug &dbg,
                 ZeroDMA &zero_dma,
                 uintptr_t addr,
                 uintptr_t size);

  int Init();

  VolatileBuffer<T> LowerHalf();
  VolatileBuffer<T> UpperHalf();

  volatile T *Data();
};

template <typename T>
VolatileBuffer<T>::VolatileBuffer(app::debug::Debug &dbg,
                                  ZeroDMA &zero_dma,
                                  uintptr_t addr,
                                  uintptr_t size)
    : dbg(dbg), zero_dma(zero_dma), addr(addr), size(size) {}

template <typename T>
int VolatileBuffer<T>::Init() {
  if (0 != zero_dma.ZeroWordsUnsafe(addr, size / sizeof(uint32_t))) {
    return 1;
  }

  return 0;
}

template <typename T>
VolatileBuffer<T> VolatileBuffer<T>::LowerHalf() {
  return VolatileBuffer<T>(dbg, zero_dma, addr, size / 2);
}

template <typename T>
VolatileBuffer<T> VolatileBuffer<T>::UpperHalf() {
  return VolatileBuffer<T>(dbg, zero_dma, addr + size / 2, size - size / 2);
}

template <typename T>
volatile T *VolatileBuffer<T>::Data() {
  return (volatile T *)addr;
}

}  // namespace app::hw
