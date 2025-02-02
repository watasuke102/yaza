#pragma once

#include <wayland-util.h>

#include <cstring>

namespace yaza::util::convert {
/// return false if fails
template <typename T>
bool from_wl_array(wl_array* array, T* dst) {
  if (array->size != sizeof(T)) {
    return false;
  }
  std::memcpy(dst, array->data, array->size);
  return true;
}
}  // namespace yaza::util::convert
