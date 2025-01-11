#pragma once

#include <wayland-server-core.h>

#include <cstdint>

namespace yaza::wayland::region {
void create(wl_client* client, uint32_t id);
}  // namespace yaza::wayland::region
