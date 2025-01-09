#pragma once

#include <sys/types.h>
#include <wayland-server.h>

namespace yaza::zwin::shm_pool {
struct ShmPool {
  wl_resource* resource_;
  int          internal_refcount_;
  int          external_refcount_;
  char*        data_;
  ssize_t      size_;
  ssize_t      new_size_;
  bool         sigbuf_is_impossible_;
};

ShmPool* new_pool(wl_client* client, wl_resource* resource, uint32_t id,
    int32_t fd, off_t size, bool sigbuf_is_impossible);
void     unref(ShmPool* pool, bool external);
}  // namespace yaza::zwin::shm_pool
