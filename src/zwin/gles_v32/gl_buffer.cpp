#include "zwin/gles_v32/gl_buffer.hpp"

#include <wayland-server-core.h>
#include <wayland-server.h>
#include <wayland-util.h>
#include <zen-remote/server/gl-buffer.h>
#include <zwin-gles-v32-protocol.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <optional>

#include "remote/loop.hpp"
#include "remote/remote.hpp"
#include "util/weak_resource.hpp"
#include "zen-remote/server/buffer.h"
#include "zwin/shm/shm_buffer.hpp"

namespace yaza::zwin::gles_v32::gl_buffer {
GlBuffer::GlBuffer(wl_event_loop* loop) : loop_(loop) {
  this->session_disconnected_listener_.set_handler(
      [this](std::nullptr_t* /*data*/) {
        this->proxy_ = std::nullopt;
      });
  remote::g_remote->listen_session_disconnected(
      this->session_disconnected_listener_);
  LOG_DEBUG("created: GlBuffer");
}
GlBuffer::~GlBuffer() {
  LOG_DEBUG("destructor: GlBuffer");
}

void GlBuffer::commit() {
  if (!this->pending_.data_.has_resource()) {
    return;
  }
  if (this->current_.data_) {
    this->current_.data_.reset();
  }
  auto* buffer         = this->pending_.data_.get_buffer();
  auto* data           = zwin::shm_buffer::get_buffer_data(buffer);
  auto  size           = zwin::shm_buffer::get_buffer_size(buffer);
  this->current_.data_ = std::shared_ptr<void>(malloc(size), free);

  zwin::shm_buffer::begin_access(buffer);
  std::memcpy(this->current_.data_.get(), data, size);
  zwin::shm_buffer::end_access(buffer);

  this->current_.data_damaged_ = true;
  this->current_.target_       = this->pending_.target_;
  this->current_.usage_        = this->pending_.usage_;
  this->current_.data_size_    = size;

  this->pending_.data_.zwn_buffer_send_release();
  this->pending_.data_.unlink();
}

void GlBuffer::sync(bool force_sync) {
  if (!this->proxy_.has_value()) {
    this->proxy_ = zen::remote::server::CreateGlBuffer(
        remote::g_remote->channel_nonnull());
  }
  if (!force_sync && !this->current_.data_damaged_) {
    return;
  }
  auto data(this->current_.data_);
  auto buffer = zen::remote::server::CreateBuffer(
      data.get(),
      [data = std::move(data)]() mutable {
        data.reset();
      },
      std::make_unique<remote::Loop>(loop_));
  this->proxy_->get()->GlBufferData(std::move(buffer), this->current_.target_,
      this->current_.data_size_, this->current_.usage_);
  this->current_.data_damaged_ = false;
}

void GlBuffer::update_pending_data(
    uint32_t target, uint32_t usage, wl_resource* data) {
  this->pending_.target_ = target;
  this->pending_.usage_  = usage;
  this->pending_.data_.link(data);
}

namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void data(wl_client* /*client*/, wl_resource* resource, uint32_t target,
    wl_resource* data, uint32_t usage) {
  auto* self = static_cast<GlBuffer*>(wl_resource_get_user_data(resource));
  self->update_pending_data(target, usage, data);
}
const struct zwn_gl_buffer_interface kImpl = {
    .destroy = destroy,
    .data    = data,
};

void destroy(wl_resource* resource) {
  auto* self = static_cast<GlBuffer*>(wl_resource_get_user_data(resource));
  delete self;
}
}  // namespace
void create(wl_client* client, uint32_t id, wl_event_loop* loop) {
  wl_resource* resource =
      wl_resource_create(client, &zwn_gl_buffer_interface, 1, id);
  if (!resource) {
    wl_client_post_no_memory(client);
    return;
  }
  auto* self = new GlBuffer(loop);
  wl_resource_set_implementation(resource, &kImpl, self, destroy);
}
}  // namespace yaza::zwin::gles_v32::gl_buffer
