#pragma once

#include <wayland-server.h>

#include <type_traits>

#include "zwin/shm/shm_buffer.hpp"

namespace yaza::util {
template <typename T>
class WeakResource {
  static_assert(std::is_pointer_v<T>);

 public:
  WeakResource() {
    this->resource_destroy_listener_.notify = [](wl_listener* listener,
                                                  void* /*data*/) {
      WeakResource* self =
          wl_container_of(listener, self, resource_destroy_listener_);
      self->unlink();
    };
    wl_list_init(&this->resource_destroy_listener_.link);
  }
  WeakResource(const WeakResource<T>& other) : WeakResource() {
    this->link(other.resource_);
  }
  WeakResource(WeakResource<T>&& other) noexcept : WeakResource() {
    this->link(other.resource_);
  }
  WeakResource<T>& operator=(const WeakResource<T>& other) {
    if (this != &other) {
      this->link(other.resource_);
    }
    return *this;
  }
  WeakResource<T>& operator=(WeakResource<T>&& other) noexcept {
    if (this != &other) {
      this->link(other.resource_);
    }
    return *this;
  }
  ~WeakResource() {
    this->unlink();
  }

  void link(wl_resource* resource) {
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
  void unlink() {
    this->link(nullptr);
  }

  bool has_resource() {
    return this->resource_ != nullptr;
  }
  T get_user_data() const {
    if (this->resource_) {
      return static_cast<T>(wl_resource_get_user_data(this->resource_));
    }
    return nullptr;
  }
  zwin::shm_buffer::ShmBuffer* get_buffer() const {
    return zwin::shm_buffer::get_buffer(this->resource_);
  }
  void zwn_buffer_send_release() {
    if (this->has_resource()) {
      ::zwn_buffer_send_release(this->resource_);
    }
  }

 private:
  wl_resource* resource_ = nullptr;
  wl_listener  resource_destroy_listener_;
};

}  // namespace yaza::util
