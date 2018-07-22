#include <stdint.h>

#include <array.h>

#include "debug/class.h"
#include "debug/counter.h"
#include "debug/macros.h"

namespace app::debug {

Counter::Counter(app::debug::Debug& dbg, const char* name)
    : dbg(dbg), name(name) {
}

void Counter::Increment() {
  __sync_fetch_and_add(&counter, 1);
}

uint32_t Counter::GetValue() {
  return counter;
}

}  // namespace app::debug
