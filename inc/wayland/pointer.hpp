#pragma once

#include <wayland-server-core.h>

#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_int2.hpp>

namespace yaza::wayland::pointer {
void create(wl_client* client, uint32_t id);
}  // namespace yaza::wayland::pointer
