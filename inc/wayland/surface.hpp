#pragma once

#include <wayland-server-core.h>

#include <cstdint>

namespace yaza::wayland::surface {
void create(wl_client* client, wl_resource* resource, uint32_t id);
}  // namespace yaza::wayland::surface
