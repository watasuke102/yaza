#include <sys/types.h>
#include <wayland-util.h>

namespace yaza::zwin::shm_util {
/// return 0 if succeeds
int wl_array_to_off_t(wl_array* array, off_t* dst);
}  // namespace yaza::zwin::shm_util
