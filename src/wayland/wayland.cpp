#include "wayland/wayland.hpp"

#include <wayland-server-protocol.h>

#include "common.hpp"
#include "wayland/compositor.hpp"
#include "wayland/data_device/data_device_manager.hpp"
#include "wayland/output.hpp"
#include "wayland/seat.hpp"

namespace yaza::wayland {
bool init(wl_display* display) {
  if (!wl_global_create(display, &wl_compositor_interface,
          wl_compositor_interface.version, nullptr,
          wayland::compositor::bind)) {
    LOG_ERR("Failed to create global (compositor)");
    return false;
  }
  if (!wl_global_create(display, &wl_seat_interface, wl_seat_interface.version,
          nullptr, wayland::seat::bind)) {
    LOG_ERR("Failed to create global (seat)");
    return false;
  }
  if (!wl_global_create(display, &wl_output_interface,
          wl_output_interface.version, nullptr, wayland::output::bind)) {
    LOG_ERR("Failed to create global (output)");
    return false;
  }
  if (!wl_global_create(display, &wl_data_device_manager_interface,
          wl_data_device_manager_interface.version, nullptr,
          wayland::data_device_manager::bind)) {
    LOG_ERR("Failed to create global (data_device_manager)");
    return false;
  }
  return true;
}
}  // namespace yaza::wayland
