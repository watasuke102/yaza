#pragma once

#include <wayland-server-core.h>

namespace yaza::xdg_shell {
/// return false if fails
bool init(wl_display* display);
}  // namespace yaza::xdg_shell
