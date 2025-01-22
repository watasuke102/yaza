#pragma once

#include <wayland-server-core.h>

#include "server.hpp"

namespace yaza::wayland {
/// return false if fails
bool init(wl_display* display);
}  // namespace yaza::wayland
