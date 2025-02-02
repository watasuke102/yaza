#include "zwin/virtual_object.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>
#include <wayland-util.h>
#include <zen-remote/server/virtual-object.h>
#include <zwin-protocol.h>

#include <cstdint>
#include <ctime>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/vec3.hpp>

#include "common.hpp"
#include "remote/session.hpp"
#include "server.hpp"
#include "util/time.hpp"

namespace yaza::zwin::virtual_object {
VirtualObject::VirtualObject(wl_resource* resource) : resource_(resource) {
  this->session_established_listener_.set_handler(
      [this](remote::Session* /*data*/) {
        if (this->committed_) {
          // force sync everything when the session is established
          this->sync(true);
        }
      });
  server::get().remote->listen_session_established(
      this->session_established_listener_);

  this->session_disconnected_listener_.set_handler(
      [this](std::nullptr_t* /*data*/) {
        this->proxy_ = std::nullopt;
      });
  server::get().remote->listen_session_disconnected(
      this->session_disconnected_listener_);

  this->session_frame_listener_.set_handler([this](std::nullptr_t* /*data*/) {
    auto         now_msec = util::now_msec();
    wl_resource* callback = nullptr;
    wl_resource* tmp      = nullptr;
    wl_resource_for_each_safe(
        callback, tmp, &this->current_.frame_callback_list) {
      wl_callback_send_done(callback, now_msec);
      wl_resource_destroy(callback);
    }
  });
  server::get().remote->listen_session_frame(this->session_frame_listener_);

  wl_list_init(&this->pending_.frame_callback_list);
  wl_list_init(&this->current_.frame_callback_list);
  LOG_DEBUG("created: VirtualObject");
}
VirtualObject::~VirtualObject() {
  LOG_DEBUG("destructor: VirtualObject");
  wl_list_remove(&this->pending_.frame_callback_list);
  wl_list_remove(&this->current_.frame_callback_list);
  this->destroying_ = true;  // disable removing RenderingUnit from list
  for (auto* unit : this->rendering_unit_list_) {
    delete unit;
  }
  if (this->app_.has_value()) {
    delete this->app_.value();
  }
}

void VirtualObject::commit() {
  wl_list_insert_list(
      &this->current_.frame_callback_list, &this->pending_.frame_callback_list);
  wl_list_init(&this->pending_.frame_callback_list);
  this->committed_ = true;
  this->events_.committed.emit(nullptr);
  if (server::get().remote->has_session()) {
    // sync only updated (damaged) data
    this->sync(false);
  }
}

// FIXME: give `channel` to all child `sync()` API
// so that callee will not use `channel_nonnull()`
/// The top level of call tree that sync data by zen-remote
void VirtualObject::sync(bool force_sync) {
  if (!this->app_) {
    LOG_WARN("VirtualObject is syncing but app is not attached");
    return;
  }
  if (!this->proxy_.has_value()) {
    this->proxy_ = zen::remote::server::CreateVirtualObject(
        server::get().remote->channel_nonnull());
    auto geom = (*this->app_)->get()->geometry();
    this->proxy_->get()->Move(
        glm::value_ptr(geom.pos()), glm::value_ptr(geom.rot()));
  }
  for (auto* unit : this->rendering_unit_list_) {
    unit->sync(force_sync);
  }
  this->proxy_->get()->Commit();
}

void VirtualObject::set_app(
    std::optional<util::UniPtr<input::BoundedObject>*> app) {
  this->app_ = app;
}
void VirtualObject::add_rendering_unit(
    gles_v32::rendering_unit::RenderingUnit* unit) {
  this->rendering_unit_list_.emplace_back(unit);
}
void VirtualObject::remove_rendering_unit(
    gles_v32::rendering_unit::RenderingUnit* unit) {
  // ignore removing request when destroying
  // (when RenderingUnit is deleted by `this`)
  if (!this->destroying_) {
    this->rendering_unit_list_.remove(unit);
  }
}
void VirtualObject::queue_frame_callback(wl_resource* callback_resource) const {
  wl_list_insert(this->pending_.frame_callback_list.prev,
      wl_resource_get_link(callback_resource));
}

void VirtualObject::listen_commited(util::Listener<std::nullptr_t*>& listener) {
  this->events_.committed.add_listener(listener);
}

namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void commit(wl_client* /*client*/, wl_resource* resource) {
  auto* self = static_cast<VirtualObject*>(wl_resource_get_user_data(resource));
  self->commit();
}
void frame(wl_client* client, wl_resource* resource, uint32_t callback) {
  wl_resource* callback_resource =
      wl_resource_create(client, &wl_callback_interface, 1, callback);
  if (!callback_resource) {
    wl_resource_post_no_memory(resource);
    return;
  }
  wl_resource_set_implementation(
      callback_resource, nullptr, nullptr, [](wl_resource* resource) {
        wl_list_remove(wl_resource_get_link(resource));
      });

  auto* self = static_cast<VirtualObject*>(wl_resource_get_user_data(resource));
  self->queue_frame_callback(callback_resource);
}
constexpr struct zwn_virtual_object_interface kImpl = {
    .destroy = destroy,
    .commit  = commit,
    .frame   = frame,
};

void destroy(wl_resource* resource) {
  auto* self = static_cast<VirtualObject*>(wl_resource_get_user_data(resource));
  delete self;
}
}  // namespace

void create(wl_client* client, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &zwn_virtual_object_interface, 1, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  auto* self = new VirtualObject(resource);
  wl_resource_set_implementation(resource, &kImpl, self, destroy);
}
}  // namespace yaza::zwin::virtual_object
