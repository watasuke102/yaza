#pragma once

#include <zen-remote/server/buffer.h>

#include <cstring>
#include <memory>

#include "remote/loop.hpp"
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

  std::unique_ptr<zen::remote::server::IBuffer> create_buffer(
      wl_event_loop* loop) {
    auto data(this->data_);
    return zen::remote::server::CreateBuffer(
        data.get(),
        [data = std::move(data)]() mutable {
          data.reset();
        },
        std::make_unique<remote::Loop>(loop));
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
