#pragma once

#include <array.h>
#include <container.h>

#include "debug/class.h"

namespace app::debug {

class Counter {
 private:
  app::debug::Debug& dbg;

  volatile uint32_t counter = 0;

 public:
  const char* const name;
  Counter(app::debug::Debug& dbg, const char* name);
  void Increment();
  uint32_t GetValue();
};

}  // namespace app::debug
