#pragma once

#include <wayland-server-core.h>

#include <cstdint>

namespace yaza::region {
void new_region(wl_client* client, uint32_t id);
}  // namespace yaza::region
