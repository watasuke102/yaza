#include "util/weak_resource.hpp"

#include <wayland-server-core.h>
#include <wayland-server.h>
#include <wayland-util.h>
#include <zwin-protocol.h>

#include "zwin/shm/shm_buffer.hpp"

namespace yaza::util {
WeakResource::WeakResource() {
  this->resource_destroy_listener_.notify = [](wl_listener* listener,
                                                void* /*data*/) {
    WeakResource* self =
        wl_container_of(listener, self, resource_destroy_listener_);
    self->unlink();
  };
  wl_list_init(&this->resource_destroy_listener_.link);
}

WeakResource::~WeakResource() {
  this->unlink();
}

void WeakResource::link(wl_resource* resource) {
  if (this->resource_) {
    wl_list_remove(&this->resource_destroy_listener_.link);
    wl_list_init(&this->resource_destroy_listener_.link);
  }
  if (resource) {
    wl_resource_add_destroy_listener(
        resource, &this->resource_destroy_listener_);
  }
  this->resource_ = resource;
}
void WeakResource::unlink() {
  this->link(nullptr);
}

bool WeakResource::has_resource() {
  return this->resource_ != nullptr;
}
void* WeakResource::get_user_data() {
  if (this->resource_) {
    return wl_resource_get_user_data(this->resource_);
  }
  return nullptr;
}
zwin::shm_buffer::ShmBuffer* WeakResource::get_buffer() {
  return zwin::shm_buffer::get_buffer(this->resource_);
}
void WeakResource::zwn_buffer_send_release() {
  if (this->has_resource()) {
    ::zwn_buffer_send_release(this->resource_);
  }
}
}  // namespace yaza::util
