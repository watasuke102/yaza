#pragma once

#include <wayland-server-core.h>

namespace yaza::wayland::data_device_manager {
void bind(wl_client* client, void* data, uint32_t version, uint32_t id);
}
