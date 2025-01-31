#pragma once

#include <wayland-server-protocol.h>

namespace yaza::xdg_shell::xdg_popup {
void create(wl_client* client, uint32_t id);
}
