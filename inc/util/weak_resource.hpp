#pragma once

#include <wayland-server.h>

#include "zwin/shm/shm_buffer.hpp"

namespace yaza::util {
class WeakResource {
 public:
  WeakResource();
  WeakResource(const WeakResource&);
  WeakResource(WeakResource&&) noexcept;
  WeakResource& operator=(const WeakResource&);
  WeakResource& operator=(WeakResource&&) noexcept;
  ~WeakResource();

  void link(wl_resource* resource);
  void unlink();

  bool                         has_resource();
  void*                        get_user_data();
  zwin::shm_buffer::ShmBuffer* get_buffer();
  void                         zwn_buffer_send_release();

 private:
  wl_resource* resource_ = nullptr;
  wl_listener  resource_destroy_listener_;
};
}  // namespace yaza::util
