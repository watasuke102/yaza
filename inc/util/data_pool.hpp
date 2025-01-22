#pragma once

#include <wayland-server-core.h>
#include <zen-remote/server/buffer.h>

#include <cstring>
#include <memory>

#include "remote/loop.hpp"
#include "server.hpp"
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
  ~DataPool() {
  }

  void from_weak_resource(const WeakResource<void*>& data) {
    auto* buffer   = data.get_buffer();
    auto* data_ptr = zwin::shm_buffer::get_buffer_data(buffer);
    this->size_    = zwin::shm_buffer::get_buffer_size(buffer);
    this->data_    = std::shared_ptr<void>(malloc(this->size_), free);

    zwin::shm_buffer::begin_access(buffer);
    std::memcpy(this->data_.get(), data_ptr, this->size_);
    zwin::shm_buffer::end_access(buffer);
  }

  void from_wl_shm_buffer(wl_shm_buffer* buffer) {
    this->size_ = static_cast<ssize_t>(wl_shm_buffer_get_stride(buffer)) *
                  wl_shm_buffer_get_height(buffer);
    this->data_    = std::shared_ptr<void>(malloc(this->size_), free);
    auto* data_ptr = wl_shm_buffer_get_data(buffer);

    wl_shm_buffer_begin_access(buffer);
    std::memcpy(this->data_.get(), data_ptr, this->size_);
    wl_shm_buffer_end_access(buffer);
  }

  std::unique_ptr<zen::remote::server::IBuffer> create_buffer() {
    auto data(this->data_);
    return zen::remote::server::CreateBuffer(
        data.get(),
        [data = std::move(data)]() mutable {
          data.reset();
        },
        std::make_unique<remote::Loop>(server::loop()));
  }

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
};
}  // namespace yaza::util
