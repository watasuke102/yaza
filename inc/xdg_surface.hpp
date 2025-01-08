#pragma once
#include <wayland-server-protocol.h>

namespace yaza::xdg_surface {
void new_xdg_surface(wl_client* client, int version, uint32_t id);
}
