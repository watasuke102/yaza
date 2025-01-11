#pragma once

#include <wayland-server-core.h>

namespace yaza::zwin::gles_v32::gl_buffer {
void create(wl_client* client, uint32_t id, wl_event_loop* loop);
}  // namespace yaza::zwin::gles_v32::gl_buffer
