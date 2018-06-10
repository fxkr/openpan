#pragma once

#include "debug/class.h"
#include "debug/macros.h"

#include "volatile_buffer.h"

namespace app::hw {

template <typename T>
class VolatileTripleBuffer {
 private:
  app::debug::Debug& dbg;

  // Accessible by hardware
  VolatileBuffer<T>* volatile front;

  // Ready to swap by code or hardware
  VolatileBuffer<T>* volatile next_front;

  // Accessible by code
  VolatileBuffer<T>* volatile back;

 public:
  uint32_t const size;

  VolatileTripleBuffer(app::debug::Debug& dbg,
                       VolatileBuffer<T>& front,
                       VolatileBuffer<T>& next_front,
                       VolatileBuffer<T>& back);

  int Init();

  // Exchange front buffer with next front buffer
  void FlipFrontBuffer();

  // Exchange back buffer with next front buffer
  void FlipBackBuffer();

  // Current buffer for hardware access
  VolatileBuffer<T>& GetFrontBuffer();

  // Currently unused buffer
  VolatileBuffer<T>& GetNextFrontBuffer();

  // Current buffer for code access
  VolatileBuffer<T>& GetBackBuffer();
};

template <typename T>
VolatileTripleBuffer<T>::VolatileTripleBuffer(app::debug::Debug& dbg,
                                              VolatileBuffer<T>& front,
                                              VolatileBuffer<T>& next_front,
                                              VolatileBuffer<T>& back)
    : dbg(dbg),
      front(&front),
      next_front(&next_front),
      back(&back),
      size(front.size) {
  crash_if_not(dbg, size == front.size);
  crash_if_not(dbg, size == next_front.size);
  crash_if_not(dbg, size == back.size);
}

template <typename T>
int VolatileTripleBuffer<T>::Init() {
  return 0;
}

template <typename T>
void VolatileTripleBuffer<T>::FlipFrontBuffer() {
  VolatileBuffer<T>* volatile orig_next_front = next_front;
  next_front = front;
  front = orig_next_front;
}

template <typename T>
void VolatileTripleBuffer<T>::FlipBackBuffer() {
  VolatileBuffer<T>* volatile orig_next_front = next_front;
  next_front = back;
  back = orig_next_front;
}

template <typename T>
VolatileBuffer<T>& VolatileTripleBuffer<T>::GetFrontBuffer() {
  return *front;
}

template <typename T>
VolatileBuffer<T>& VolatileTripleBuffer<T>::GetBackBuffer() {
  return *back;
}

template <typename T>
VolatileBuffer<T>& VolatileTripleBuffer<T>::GetNextFrontBuffer() {
  return *next_front;
}

}  // namespace app::hw
