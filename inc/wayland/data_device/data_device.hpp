#pragma once

#include <wayland-server-core.h>
#include <wayland-server.h>

namespace yaza::wayland::data_device {
wl_resource* create(wl_client* client, uint32_t id);
}
