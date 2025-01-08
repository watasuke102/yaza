#pragma once

#include <wayland-server-core.h>

#include <cstdint>

namespace yaza::zwin::virtual_object {
void create(wl_client* client, uint32_t id);
}
