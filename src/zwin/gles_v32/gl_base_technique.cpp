#include "zwin/gles_v32/gl_base_technique.hpp"

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

#include "remote/remote.hpp"
#include "zwin/gles_v32/rendering_unit.hpp"

namespace yaza::zwin::gles_v32::gl_base_technique {
GlBaseTechnique::GlBaseTechnique(
    wl_resource* resource, rendering_unit::RenderingUnit* unit)
    : owner_(unit), resource_(resource) {
  this->session_disconnected_listener_.set_handler(
      [this](std::nullptr_t* /*data*/) {
        this->proxy_ = std::nullopt;
      });
  remote::g_remote->listen_session_disconnected(
      this->session_disconnected_listener_);

  this->owner_committed_listener_.set_handler([this](std::nullptr_t* /*data*/) {
    this->commit();
  });
  this->owner_->listen_commited(this->owner_committed_listener_);

  LOG_DEBUG("created: GlBaseTechnique");
}
GlBaseTechnique::~GlBaseTechnique() {
  LOG_DEBUG("destructor: GlBaseTechnique");
  this->owner_->set_technique(std::nullopt);
  wl_resource_set_user_data(this->resource_, nullptr);
  wl_resource_set_destructor(this->resource_, nullptr);
}

void GlBaseTechnique::commit() {
  if (!this->commited_) {
    this->owner_->set_technique(this);
    this->commited_ = true;
  }
}

void GlBaseTechnique::sync(bool force_sync) {
  if (!this->proxy_.has_value()) {
    this->proxy_ = zen::remote::server::CreateGlBaseTechnique(
        remote::g_remote->channel_nonnull(), this->owner_->remote_id());
  }
}

namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void bind_program(
    wl_client* client, wl_resource* resource, wl_resource* program) {
}
void bind_vertex_array(
    wl_client* client, wl_resource* resource, wl_resource* vertex_array) {
}
void bind_texture(wl_client* client, wl_resource* resource, uint32_t binding,
    const char* name, wl_resource* texture, uint32_t target,
    wl_resource* sampler) {
}
void uniform_vector(wl_client* client, wl_resource* resource, uint32_t location,
    const char* name, uint32_t type, uint32_t size, uint32_t count,
    wl_array* value) {
}
void uniform_matrix(wl_client* client, wl_resource* resource, uint32_t location,
    const char* name, uint32_t col, uint32_t row, uint32_t count,
    uint32_t transpose, wl_array* value) {
}
void draw_arrays(wl_client* client, wl_resource* resource, uint32_t mode,
    int32_t first, uint32_t count) {
}
void draw_elements(wl_client* client, wl_resource* resource, uint32_t mode,
    uint32_t count, uint32_t type, wl_array* offset,
    wl_resource* element_array_buffer) {
}
const struct zwn_gl_base_technique_interface kImpl = {
    .destroy           = destroy,
    .bind_program      = bind_program,
    .bind_vertex_array = bind_vertex_array,
    .bind_texture      = bind_texture,
    .uniform_vector    = uniform_vector,
    .uniform_matrix    = uniform_matrix,
    .draw_arrays       = draw_arrays,
    .draw_elements     = draw_elements,
};  // namespace

void destroy(wl_resource* resource) {
  auto* self =
      static_cast<GlBaseTechnique*>(wl_resource_get_user_data(resource));
  delete self;
}
}  // namespace
void create(
    wl_client* client, uint32_t id, rendering_unit::RenderingUnit* unit) {
  wl_resource* resource =
      wl_resource_create(client, &zwn_gl_base_technique_interface, 1, id);
  if (!resource) {
    wl_client_post_no_memory(client);
    return;
  }
  auto* self = new GlBaseTechnique(resource, unit);
  wl_resource_set_implementation(resource, &kImpl, self, destroy);
}
}  // namespace yaza::zwin::gles_v32::gl_base_technique
