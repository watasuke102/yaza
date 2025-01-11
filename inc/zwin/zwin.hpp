#pragma once

#include <wayland-server-core.h>

#include "server.hpp"

namespace yaza::zwin {
/// return false if fails
bool init(wl_display* display, Server* server);
}  // namespace yaza::zwin
