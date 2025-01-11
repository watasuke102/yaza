#pragma once

#include <cstdint>

#include "wayland-server-core.h"

namespace yaza::wayland::compositor {
void bind(wl_client* client, void* data, uint32_t version, uint32_t id);
}
