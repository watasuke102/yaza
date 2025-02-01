#pragma once

#include <wayland-server-core.h>

namespace yaza::wayland::pointer {
void create(wl_client* client, uint32_t id);
}
