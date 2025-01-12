#include "zwin/gles_v32/rendering_unit.hpp"

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
#include "zwin/virtual_object.hpp"

namespace yaza::zwin::gles_v32::rendering_unit {
RenderingUnit::RenderingUnit(
    wl_resource* resource, virtual_object::VirtualObject* virtual_object)
    : owner_(virtual_object), resource_(resource) {
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

  LOG_DEBUG("created: RenderingUnit");
}
RenderingUnit::~RenderingUnit() {
  LOG_DEBUG("destructor: RenderingUnit");
  this->owner_->remove_rendering_unit(this);
  if (this->technique_.has_value()) {
    delete this->technique_.value();
  }
  wl_resource_set_user_data(this->resource_, nullptr);
  wl_resource_set_destructor(this->resource_, nullptr);
}

void RenderingUnit::commit() {
  if (!this->committed_) {
    this->owner_->add_rendering_unit(this);
    this->committed_ = true;
  }
  this->events_.committed_.emit(nullptr);
}

void RenderingUnit::sync(bool force_sync) {
  if (!this->proxy_.has_value()) {
    this->proxy_ = zen::remote::server::CreateRenderingUnit(
        remote::g_remote->channel_nonnull(), this->owner_->remote_id());
  }
  if (this->technique_.has_value()) {
    this->technique_.value()->sync(force_sync);
  }
}

void RenderingUnit::set_technique(
    std::optional<gl_base_technique::GlBaseTechnique*> technique) {
  if (technique.has_value() && this->technique_.has_value()) {
    wl_resource_post_error(this->resource_,
        ZWN_GL_BASE_TECHNIQUE_ERROR_TECHNIQUE,
        "new (nonnull) technique tried to set, "
        "but RenderingUnit already has another technique");
    return;
  }
  this->technique_ = technique;
}

void RenderingUnit::listen_commited(util::Listener<std::nullptr_t*>& listener) {
  this->events_.committed_.add_listener(listener);
}

namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
const struct zwn_rendering_unit_interface kImpl = {
    .destroy = destroy,
};

void destroy(wl_resource* resource) {
  auto* self = static_cast<RenderingUnit*>(wl_resource_get_user_data(resource));
  delete self;
}
}  // namespace
void create(wl_client* client, uint32_t id,
    virtual_object::VirtualObject* virtual_object) {
  wl_resource* resource =
      wl_resource_create(client, &zwn_rendering_unit_interface, 1, id);
  if (!resource) {
    wl_client_post_no_memory(client);
    return;
  }
  auto* self = new RenderingUnit(resource, virtual_object);
  wl_resource_set_implementation(resource, &kImpl, self, destroy);
}
}  // namespace yaza::zwin::gles_v32::rendering_unit
