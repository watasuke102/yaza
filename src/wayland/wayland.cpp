#include "wayland/wayland.hpp"

#include "common.hpp"
#include "wayland/compositor.hpp"

namespace yaza::wayland {
bool init(wl_display* display, Server* server) {
  // NOLINTNEXTLINE(readability-magic-numbers)
  if (!wl_global_create(display, &wl_compositor_interface, 5, server,
          wayland::compositor::bind)) {
    LOG_ERR("Failed to create global (compositor)");
    return false;
  }
  return true;
}
}  // namespace yaza::wayland
