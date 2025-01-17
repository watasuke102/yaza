#pragma once

#include <wayland-util.h>

#include <cstring>

namespace yaza::util {
/// return false if succeeds
template <typename T>
bool convert_wl_array(wl_array* array, T* dst) {
  if (array->size != sizeof(T)) {
    return true;
  }
  std::memcpy(dst, array->data, array->size);
  return false;
}
}  // namespace yaza::util
