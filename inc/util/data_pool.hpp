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

  /// read wl_shm_buffer attached to wl_surface, convert RGBA format and store
  void read_wl_surface_texture(wl_shm_buffer* buffer) {
    this->size_ = static_cast<ssize_t>(wl_shm_buffer_get_stride(buffer)) *
                  wl_shm_buffer_get_height(buffer);
    this->data_ = std::shared_ptr<void>(malloc(this->size_), free);
    auto* src   = static_cast<uint8_t*>(wl_shm_buffer_get_data(buffer));
    auto* dst   = static_cast<uint8_t*>(this->data_.get());

    // format of wl_shm_buffer matches /WL_SHM_FORMAT_[AX]RGB8888/
    // it is expressed in little-endian, so:
    // Wayland: B. G, R, A
    // OpenGL : R, G, B, A (GL_RGBA is specified in Renderer::set_texture)
    wl_shm_buffer_begin_access(buffer);
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    for (int i = 0; i < this->size_; i += 4) {
      dst[i + 0] = src[i + 2];  // R
      dst[i + 1] = src[i + 1];  // G
      dst[i + 2] = src[i + 0];  // B
      dst[i + 3] = src[i + 3];  // A
    }
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    wl_shm_buffer_end_access(buffer);
  }

  void from_ptr(const void* data, ssize_t size) {
    this->size_ = size;
    this->data_ = std::shared_ptr<void>(malloc(this->size_), free);
    std::memcpy(this->data_.get(), data, this->size_);
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
