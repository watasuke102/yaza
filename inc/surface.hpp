#pragma once

#include <wayland-server-core.h>

#include <cstdint>

namespace yaza::surface {
void new_surface(wl_client* client, wl_resource* resource, uint32_t id);
}  // namespace yaza::surface
