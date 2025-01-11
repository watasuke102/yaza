#include "zwin/zwin.hpp"

#include <zwin-gles-v32-protocol.h>
#include <zwin-protocol.h>
#include <zwin-shell-protocol.h>

#include "common.hpp"
#include "zwin/compositor.hpp"
#include "zwin/gles_v32/gles_v32.hpp"
#include "zwin/seat.hpp"
#include "zwin/shell.hpp"
#include "zwin/shm/shm.hpp"

namespace yaza::zwin {
bool init(wl_display* display, Server* server) {
  if (!wl_global_create(display, &zwn_compositor_interface, 1, server,
          zwin::compositor::bind)) {
    LOG_ERR("Failed to create global (zwin_compositor)");
    return false;
  }
  if (!wl_global_create(
          display, &zwn_seat_interface, 1, server, zwin::seat::bind)) {
    LOG_ERR("Failed to create global (zwin_seat)");
    return false;
  }
  if (!wl_global_create(
          display, &zwn_shell_interface, 1, server, zwin::shell::bind)) {
    LOG_ERR("Failed to create global (zwin_shell)");
    return false;
  }
  if (!wl_global_create(
          display, &zwn_shm_interface, 1, server, zwin::shm::bind)) {
    LOG_ERR("Failed to create global (zwin_shm)");
    return false;
  }
  if (!wl_global_create(
          display, &zwn_gles_v32_interface, 1, server, zwin::gles_v32::bind)) {
    LOG_ERR("Failed to create global (zwin_gles_v32)");
    return false;
  }
  return true;
}
}  // namespace yaza::zwin
