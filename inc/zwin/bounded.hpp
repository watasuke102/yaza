#pragma once

#include <wayland-server-core.h>

#include <cstdint>

namespace yaza::zwin::bounded {
wl_resource* create(wl_client* client, uint32_t id);
}
