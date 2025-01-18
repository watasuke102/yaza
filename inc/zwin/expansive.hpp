#pragma once

#include <wayland-server-core.h>

#include <cstdint>

#include "zwin/virtual_object.hpp"

namespace yaza::zwin::expansive {
wl_resource* create(wl_client* client, uint32_t id,
    virtual_object::VirtualObject* virtual_object);
}
