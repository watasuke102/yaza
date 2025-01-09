#include "zwin/shm/shm_util.hpp"

#include <cstring>

namespace yaza::zwin::shm_util {
/// return 0 if succeeds
int wl_array_to_off_t(wl_array* array, off_t* dst) {
  if (array->size != sizeof(int32_t) && array->size != sizeof(int64_t)) {
    return 1;
  }
  std::memcpy(dst, array->data, array->size);
  return 0;
}
}  // namespace yaza::zwin::shm_util
