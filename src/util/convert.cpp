#include "util/convert.hpp"

#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace yaza::util::convert {
template <>
bool from_wl_array(wl_array* array, off_t* dst) {
  if (array->size != sizeof(int32_t) && array->size != sizeof(int64_t)) {
    return false;
  }
  std::memcpy(dst, array->data, array->size);
  return true;
}
}  // namespace yaza::util::convert
