#pragma once

#include <wayland-server-core.h>

namespace yaza::wayland {
/// return false if fails
bool init(wl_display* display);
}  // namespace yaza::wayland
