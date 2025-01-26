#pragma once

#include <wayland-server-core.h>

namespace yaza::wayland::seat::pointer {
void create(wl_client* client, uint32_t id);
}
