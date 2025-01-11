#pragma once

#include <wayland-server-protocol.h>

namespace yaza::xdg_shell::xdg_toplevel {
void create(wl_client* client, int version, uint32_t id);
}
