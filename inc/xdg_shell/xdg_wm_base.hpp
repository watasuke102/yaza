#pragma once

#include <wayland-server-core.h>

namespace yaza::xdg_shell::xdg_wm_base {
void bind(wl_client* client, void* data, uint32_t version, uint32_t id);
}
