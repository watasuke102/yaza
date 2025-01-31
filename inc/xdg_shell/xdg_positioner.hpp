#pragma once

#include <wayland-server-protocol.h>

namespace yaza::xdg_shell::xdg_positioner {
void create(wl_client* client, uint32_t id);
}
