#pragma once

#include <wayland-server.h>

namespace yaza::wayland::keyboard {
void create(wl_client* client, uint32_t id);
}
