#pragma once

#include <wayland-server-core.h>

namespace yaza::zwin {
/// return false if fails
bool init(wl_display* display);
}  // namespace yaza::zwin
