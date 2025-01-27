#include "util/data_pool.hpp"

#include <wayland-server-core.h>
#include <zen-remote/server/buffer.h>

#include <cstdlib>
#include <cstring>
#include <memory>

#include "remote/loop.hpp"
#include "server.hpp"
#include "util/weak_resource.hpp"

namespace yaza::util {
void DataPool::from_weak_resource(const WeakResource<void*>& data) {
  auto* buffer = data.get_buffer();
  this->ensure_and_set_data_size(zwin::shm_buffer::get_buffer_size(buffer));
  auto* data_ptr = zwin::shm_buffer::get_buffer_data(buffer);

  zwin::shm_buffer::begin_access(buffer);
  std::memcpy(this->data_.get(), data_ptr, this->size_);
  zwin::shm_buffer::end_access(buffer);
}

/// read wl_shm_buffer attached to wl_surface, convert RGBA format and store
void DataPool::read_wl_surface_texture(wl_shm_buffer* buffer) {
  this->ensure_and_set_data_size(
      static_cast<ssize_t>(wl_shm_buffer_get_stride(buffer)) *
      wl_shm_buffer_get_height(buffer));
  auto* src = static_cast<uint8_t*>(wl_shm_buffer_get_data(buffer));
  auto* dst = static_cast<uint8_t*>(this->data_.get());

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

void DataPool::from_ptr(const void* data, ssize_t size) {
  this->ensure_and_set_data_size(size);
  std::memcpy(this->data_.get(), data, this->size_);
}

std::unique_ptr<zen::remote::server::IBuffer> DataPool::create_buffer() {
  auto data(this->data_);
  return zen::remote::server::CreateBuffer(
      data.get(),
      [data = std::move(data)]() mutable {
        data.reset();
      },
      std::make_unique<remote::Loop>(server::get().loop()));
}
/// renew `size_` and reallocate `data_` if the capacity is not enough
void DataPool::ensure_and_set_data_size(ssize_t size) {
  if (this->size_ >= size) {
    this->size_ = size;
    return;
  }
  if (this->has_data()) {
    this->data_.reset();
  }
  this->data_ = std::shared_ptr<void>(malloc(size), free);
  this->size_ = size;
}
}  // namespace yaza::util
