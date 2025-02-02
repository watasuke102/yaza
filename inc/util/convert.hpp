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

/// return false if fails
template <typename T>
bool to_wl_array(T* src, wl_array* array) {
  auto* dst = static_cast<T*>(wl_array_add(array, sizeof(T)));
  if (!dst) {
    return false;
  }
  std::memcpy(dst, src, sizeof(T));
  return true;
}
}  // namespace yaza::util::convert
