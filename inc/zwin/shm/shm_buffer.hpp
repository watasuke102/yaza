#pragma once

#include <sys/types.h>
#include <wayland-server.h>
#include <zwin-protocol.h>

#include "zwin/shm/shm_pool.hpp"

namespace yaza::zwin::shm_buffer {
struct ShmBuffer;

ShmBuffer* new_buffer(wl_client* client, uint32_t id, shm_pool::ShmPool* pool,
    off_t size, off_t offset);
void       begin_access(ShmBuffer* buffer);
void       end_access(ShmBuffer* buffer);
ShmBuffer* get_buffer(wl_resource* resource);
void*      get_buffer_data(ShmBuffer* buffer);
ssize_t    get_buffer_size(ShmBuffer* buffer);
}  // namespace yaza::zwin::shm_buffer
