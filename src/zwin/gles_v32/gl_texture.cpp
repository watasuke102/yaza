#include "zwin/gles_v32/gl_texture.hpp"

#include <GLES3/gl32.h>
#include <wayland-server-core.h>
#include <wayland-server.h>
#include <wayland-util.h>
#include <zen-remote/server/gl-texture.h>
#include <zwin-gles-v32-protocol.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <optional>

#include "common.hpp"
#include "remote/remote.hpp"
#include "server.hpp"
#include "util/weakable_unique_ptr.hpp"

namespace yaza::zwin::gles_v32::gl_texture {
GlTexture::GlTexture() {
  this->session_disconnected_listener_.set_handler(
      [this](std::nullptr_t* /*data*/) {
        this->proxy_ = std::nullopt;
      });
  server::get().remote->listen_session_disconnected(
      this->session_disconnected_listener_);
  LOG_DEBUG("created: GlTexture");
}
GlTexture::~GlTexture() {
  LOG_DEBUG("destructor: GlTexture");
}

void GlTexture::commit() {
  if (this->pending_.data.has_resource()) {
    if (this->current_.data.has_data()) {
      this->current_.data.reset();
    }
    this->current_.image_2d = this->pending_.image_2d;
    this->current_.data.from_weak_resource(this->pending_.data);
    this->current_.data_changed = true;

    this->pending_.data.zwn_buffer_send_release();
    this->pending_.data.unlink();
  }

  if (this->pending_.mipmap_target != 0) {
    this->current_.mipmap_target         = this->pending_.mipmap_target;
    this->current_.mipmap_target_changed = true;
    this->pending_.mipmap_target         = 0;
  }
}

void GlTexture::sync(bool force_sync) {
  if (!this->proxy_.has_value()) {
    this->proxy_ = zen::remote::server::CreateGlTexture(
        server::get().remote->channel_nonnull());
  }
  if (force_sync || this->current_.data_changed) {
    this->proxy_->get()->GlTexImage2D(this->current_.image_2d.target,
        this->current_.image_2d.level, this->current_.image_2d.internal_format,
        this->current_.image_2d.width, this->current_.image_2d.height,
        this->current_.image_2d.border, this->current_.image_2d.format,
        this->current_.image_2d.type, this->current_.data.create_buffer());
    this->current_.data_changed = false;
  }
  if (force_sync || this->current_.mipmap_target_changed) {
    this->proxy_->get()->GlGenerateMipmap(this->current_.mipmap_target);
    this->current_.mipmap_target_changed = false;
  }
}

void GlTexture::new_image_2d(Image2dData& image_2d, wl_resource* data) {
  this->pending_.image_2d = image_2d;
  this->pending_.data.link(data);
}
void GlTexture::request_generate_mipmap(uint32_t target) {
  this->pending_.mipmap_target = target;
}

namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void image_2d(wl_client* /* client */, wl_resource* resource, uint32_t target,
    int32_t level, int32_t internal_format, uint32_t width, uint32_t height,
    int32_t border, uint32_t format, uint32_t type, wl_resource* data) {
  auto* self = static_cast<util::UniPtr<GlTexture>*>(
      wl_resource_get_user_data(resource));
  Image2dData image_2d{//
      .target          = target,
      .level           = level,
      .internal_format = internal_format,
      .width           = width,
      .height          = height,
      .border          = border,
      .format          = format,
      .type            = type};
  (*self)->new_image_2d(image_2d, data);
}
void generate_mipmap(
    wl_client* /*client*/, wl_resource* resource, uint32_t target) {
  auto* self = static_cast<util::UniPtr<GlTexture>*>(
      wl_resource_get_user_data(resource));
  (*self)->request_generate_mipmap(target);
}
constexpr struct zwn_gl_texture_interface kImpl = {
    .destroy         = destroy,
    .image_2d        = image_2d,
    .generate_mipmap = generate_mipmap,
};  // namespace

void destroy(wl_resource* resource) {
  auto* self = static_cast<util::UniPtr<GlTexture>*>(
      wl_resource_get_user_data(resource));
  delete self;
}
}  // namespace
void create(wl_client* client, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &zwn_gl_texture_interface, 1, id);
  if (!resource) {
    wl_client_post_no_memory(client);
    return;
  }
  auto* self = new util::UniPtr<GlTexture>();
  wl_resource_set_implementation(resource, &kImpl, self, destroy);
}
}  // namespace yaza::zwin::gles_v32::gl_texture
