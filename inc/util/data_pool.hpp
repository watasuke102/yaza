#pragma once

#include <wayland-server-core.h>
#include <zen-remote/server/buffer.h>

#include <memory>

#include "weak_resource.hpp"

namespace yaza::util {
class DataPool {
 public:
  DataPool()                           = default;
  DataPool(const DataPool&)            = default;
  DataPool(DataPool&&)                 = default;
  DataPool& operator=(const DataPool&) = default;
  DataPool& operator=(DataPool&&)      = default;
  explicit DataPool(std::shared_ptr<void>& p) : data_(p) {
  }
  ~DataPool() = default;

  void from_weak_resource(const WeakResource<void*>& data);
  /// read wl_shm_buffer attached to wl_surface, convert RGBA format and store
  void read_wl_surface_texture(wl_shm_buffer* buffer);
  void from_ptr(const void* data, ssize_t size);

  std::unique_ptr<zen::remote::server::IBuffer> create_buffer();

  [[nodiscard]] ssize_t size() const {
    return size_;
  }
  [[nodiscard]] bool has_data() {
    return this->data_ != nullptr;
  }
  void reset() {
    this->size_ = 0;
    this->data_.reset();
  }

 private:
  ssize_t               size_ = 0;
  std::shared_ptr<void> data_;

  /// renew `size_` and reallocate `data_` if the capacity is not enough
  void ensure_and_set_data_size(ssize_t size);
};
}  // namespace yaza::util
