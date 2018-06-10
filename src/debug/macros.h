#pragma once

#include "debug/class.h"

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

#define crash(debug_inst) \
  ((debug_inst).do_crash(__FUNCTION__, __FILE__, __LINE__))

#define crash_if(debug_inst, x) \
  ({                            \
    if (unlikely(x)) {          \
      crash(debug_inst);        \
    };                          \
    (x);                        \
  })

#define crash_if_not(debug_inst, x) crash_if((debug_inst), !(x))
