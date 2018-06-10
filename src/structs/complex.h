#pragma once

namespace app::structs {

template <typename T>
struct __attribute__((__packed__)) Complex {
  T real;
  T imag;
};

}  // namespace app::structs
