#include "zwin/virtual_object.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>
#include <wayland-util.h>
#include <zwin-protocol.h>

#include <cstdint>
#include <ctime>

#include "remote/remote.hpp"
#include "remote/session.hpp"

namespace yaza::zwin::virtual_object {
VirtualObject::VirtualObject() {
  this->session_established_listener_.set_handler(
      [this](remote::Session* /*data*/) {
        if (this->committed_) {
          // force sync everything when the session is established
          this->sync(true);
        }
      });
  remote::g_remote->listen_session_established(
      this->session_established_listener_);

  this->session_disconnected_listener_.set_handler(
      [this](std::nullptr_t* /*data*/) {
        this->proxy_ = std::nullopt;
      });
  remote::g_remote->listen_session_disconnected(
      this->session_disconnected_listener_);

  this->session_frame_listener_.set_handler([this](std::nullptr_t* /*data*/) {
    this->send_frame_done();
  });
  remote::g_remote->listen_session_frame(this->session_frame_listener_);

  wl_list_init(&this->pending_.frame_callback_list_);
  wl_list_init(&this->current_.frame_callback_list_);
  LOG_DEBUG("created: VirtualObject");
}
VirtualObject::~VirtualObject() {
  LOG_DEBUG("destructor: VirtualObject");
  wl_list_remove(&this->pending_.frame_callback_list_);
  wl_list_remove(&this->current_.frame_callback_list_);
}

void VirtualObject::commit() {
  wl_list_insert_list(&this->current_.frame_callback_list_,
      &this->pending_.frame_callback_list_);
  wl_list_init(&this->pending_.frame_callback_list_);
  this->committed_ = true;
  this->events_.begin_commit_.emit(nullptr);
  if (remote::g_remote->has_session()) {
    // sync only updated (damaged) data
    this->sync(false);
  }
}

void VirtualObject::sync(bool force_sync) {
}

void VirtualObject::queue_frame_callback(wl_resource* callback_resource) const {
  wl_list_insert(this->pending_.frame_callback_list_.prev,
      wl_resource_get_link(callback_resource));
}
void VirtualObject::send_frame_done() {
  timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
    return;
  }
  // NOLINTBEGIN(readability-magic-numbers)
  int64_t now_msec =
      (static_cast<int64_t>(ts.tv_sec) * 1000) + (ts.tv_nsec / 1'000'000);
  // NOLINTEND(readability-magic-numbers)

  wl_resource* resource = nullptr;
  wl_resource* tmp      = nullptr;
  wl_resource_for_each_safe(
      resource, tmp, &this->current_.frame_callback_list_) {
    wl_callback_send_done(resource, now_msec);
    wl_resource_destroy(resource);
  }
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
const struct zwn_virtual_object_interface kImpl = {
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
  auto* self = new VirtualObject();
  wl_resource_set_implementation(resource, &kImpl, self, destroy);
}
}  // namespace yaza::zwin::virtual_object
