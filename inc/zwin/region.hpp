#pragma once

#include <wayland-server-core.h>
#include <wayland-server.h>
#include <wayland-util.h>

#include <cstdint>

namespace yaza::zwin::region {
void create(wl_client* client, uint32_t id);
}  // namespace yaza::zwin::region
