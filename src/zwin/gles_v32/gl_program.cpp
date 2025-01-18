#include "zwin/gles_v32/gl_program.hpp"

#include <wayland-server-core.h>
#include <wayland-server.h>
#include <wayland-util.h>
#include <zen-remote/server/gl-program.h>
#include <zwin-gles-v32-protocol.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

#include "remote/remote.hpp"
#include "util/weakable_unique_ptr.hpp"
#include "zwin/gles_v32/gl_shader.hpp"

namespace yaza::zwin::gles_v32::gl_program {
GlProgram::GlProgram(wl_resource* resource) : resource_(resource) {
  this->session_disconnected_listener_.set_handler(
      [this](std::nullptr_t* /*data*/) {
        this->proxy_ = std::nullopt;
      });
  remote::g_remote->listen_session_disconnected(
      this->session_disconnected_listener_);
  LOG_DEBUG("created: GlProgram");
}
GlProgram::~GlProgram() {
  LOG_DEBUG("destructor: GlProgram");
}

void GlProgram::commit() {
  if (!this->pending_.damaged_) {
    return;
  }
  if (this->pending_.should_link_) {
    if (this->current_.linked_) {
      wl_resource_post_error(this->resource_, ZWN_GL_PROGRAM_ERROR_RELINK,
          "GlProgram has already been linked");
      return;
    }
    this->current_.should_link_ = true;
    this->current_.linked_      = true;
  }
  this->current_.shaders_.splice(
      this->current_.shaders_.end(), this->pending_.shaders_);
  this->pending_.shaders_.clear();
  this->pending_.damaged_     = false;
  this->pending_.should_link_ = false;
}

void GlProgram::sync(bool force_sync) {
  if (!this->proxy_.has_value()) {
    this->proxy_ = zen::remote::server::CreateGlProgram(
        remote::g_remote->channel_nonnull());
  }
  bool should_attach = force_sync || this->current_.should_link_;
  // for (auto* shader : this->current_.shaders_) {
  for (auto it = this->current_.shaders_.begin();
      it != this->current_.shaders_.end();) {
    auto* shader = it->lock();
    if (!shader) {
      it = this->current_.shaders_.erase(it);
      continue;
    }
    shader->sync();
    if (should_attach) {
      this->proxy_->get()->GlAttachShader(shader->remote_id());
    }
    ++it;
  }
  if (!should_attach) {
    return;
  }
  this->proxy_->get()->GlLinkProgram();
  this->current_.should_link_ = false;
}

void GlProgram::request_link() {
  this->pending_.damaged_     = true;
  this->pending_.should_link_ = true;
}
void GlProgram::attach_shader(util::WeakPtr<gl_shader::GlShader>&& shader) {
  this->pending_.damaged_ = true;
  this->pending_.shaders_.emplace_back(std::move(shader));
}

namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void attach_shader(wl_client* /*client*/, wl_resource* resource,
    wl_resource* shader_resource) {
  auto* self = static_cast<util::UniPtr<GlProgram>*>(
      wl_resource_get_user_data(resource));
  auto* shader = static_cast<util::UniPtr<gl_shader::GlShader>*>(
      wl_resource_get_user_data(shader_resource));
  if (shader->get()) {
    (*self)->attach_shader((*shader).weak());
  }
}
void link(wl_client* /*client*/, wl_resource* resource) {
  auto* self = static_cast<util::UniPtr<GlProgram>*>(
      wl_resource_get_user_data(resource));
  (*self)->request_link();
}
const struct zwn_gl_program_interface kImpl = {
    .destroy       = destroy,
    .attach_shader = attach_shader,
    .link          = link,
};  // namespace

void destroy(wl_resource* resource) {
  auto* self = static_cast<util::UniPtr<GlProgram>*>(
      wl_resource_get_user_data(resource));
  delete self;
}
}  // namespace
void create(wl_client* client, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &zwn_gl_program_interface, 1, id);
  if (!resource) {
    wl_client_post_no_memory(client);
    return;
  }
  auto* self = new util::UniPtr<GlProgram>(resource);
  wl_resource_set_implementation(resource, &kImpl, self, destroy);
}
}  // namespace yaza::zwin::gles_v32::gl_program
