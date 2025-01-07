#pragma once

#include "xdg-shell-protocol.h"

namespace yaza::xdg_shell {
void bind(wl_client* client, void* data, uint32_t version, uint32_t id);
}
