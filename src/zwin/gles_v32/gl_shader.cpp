#include "zwin/gles_v32/gl_shader.hpp"

#include <wayland-server-core.h>
#include <wayland-server.h>
#include <wayland-util.h>
#include <zen-remote/server/gl-shader.h>
#include <zwin-gles-v32-protocol.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <optional>

#include "remote/remote.hpp"
#include "zwin/gles_v32/gl_program.hpp"
#include "zwin/shm/shm_buffer.hpp"

namespace yaza::zwin::gles_v32::gl_shader {
GlShader::GlShader(
    wl_resource* resource, zwin::shm_buffer::ShmBuffer* buffer, uint32_t type)
    : type_(type), buffer_(buffer), resource_(resource) {
  this->session_disconnected_listener_.set_handler(
      [this](std::nullptr_t* /**/) {
        this->proxy_ = std::nullopt;
      });
  remote::g_remote->listen_session_disconnected(
      this->session_disconnected_listener_);
  LOG_DEBUG("created: GlShader");
}
GlShader::~GlShader() {
  LOG_DEBUG("destructor: GlShader");
  wl_resource_set_user_data(this->resource_, nullptr);
  wl_resource_set_destructor(this->resource_, nullptr);
  if (auto* owner = this->owner_.lock()) {
    owner->remove_shader(this);
  }
}
void GlShader::sync() {
  if (!this->proxy_.has_value()) {
    auto*   source = zwin::shm_buffer::get_buffer_data(this->buffer_);
    ssize_t len    = zwin::shm_buffer::get_buffer_size(this->buffer_);
    this->proxy_ =
        zen::remote::server::CreateGlShader(remote::g_remote->channel_nonnull(),
            std::string(static_cast<char*>(source), len), this->type_);
  }
}

namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
const struct zwn_gl_shader_interface kImpl = {
    .destroy = destroy,
};  // namespace

void destroy(wl_resource* resource) {
  auto* self = static_cast<GlShader*>(wl_resource_get_user_data(resource));
  delete self;
}
}  // namespace
void create(
    wl_client* client, uint32_t id, wl_resource* buffer, uint32_t type) {
  wl_resource* resource =
      wl_resource_create(client, &zwn_gl_shader_interface, 1, id);
  if (!resource) {
    wl_client_post_no_memory(client);
    return;
  }
  auto* self =
      new GlShader(resource, zwin::shm_buffer::get_buffer(buffer), type);
  wl_resource_set_implementation(resource, &kImpl, self, destroy);
}
}  // namespace yaza::zwin::gles_v32::gl_shader
