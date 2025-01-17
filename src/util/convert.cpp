#include "util/convert.hpp"

namespace yaza::util {
template <>
bool convert_wl_array(wl_array* array, off_t* dst) {
  if (array->size != sizeof(int32_t) && array->size != sizeof(int64_t)) {
    return true;
  }
  std::memcpy(dst, array->data, array->size);
  return false;
}
}  // namespace yaza::util
