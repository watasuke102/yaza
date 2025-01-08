#pragma once
#include <wayland-server-protocol.h>

namespace yaza::xdg_toplevel {
void new_xdg_toplevel(wl_client* client, int version, uint32_t id);
}
