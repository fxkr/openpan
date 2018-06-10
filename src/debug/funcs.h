#pragma once

#include <mbed.h>

#include "debug/class.h"

namespace app::debug {

void init(Debug &dbg);

void _crash(const char *func, const char *file, int line);

}  // namespace app::debug
