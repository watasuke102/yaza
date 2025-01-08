#pragma once

#include <wayland-server-core.h>

#include <cstdint>

namespace yaza::zwin::seat {
void bind(wl_client* client, void* data, uint32_t version, uint32_t id);
}
