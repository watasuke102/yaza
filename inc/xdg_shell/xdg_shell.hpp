#pragma once

#include <wayland-server-core.h>

#include "server.hpp"

namespace yaza::xdg_shell {
/// return false if fails
bool init(wl_display* display, Server* server);
}  // namespace yaza::xdg_shell
