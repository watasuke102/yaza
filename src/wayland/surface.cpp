#include "wayland/surface.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include <cstdint>
#include <optional>

#include "common.hpp"
#include "util/time.hpp"

namespace yaza::wayland::surface {
Surface::Surface(uint32_t id) : id_(id) {
  LOG_DEBUG("constructor: wl_surface@%u", this->id_);
}
Surface::~Surface() {
  LOG_DEBUG(" destructor: wl_surface@%u", this->id_);
}

void Surface::attach(wl_resource* buffer, int32_t /*sx*/, int32_t /*sy*/) {
  // x, y is discarded but `Setting anything other than 0 as x and y arguments
  // is discouraged`, so ignore them for now (TODO?)
  if (buffer == nullptr) {
    this->pending_.buffer_ = std::nullopt;
  } else {
    this->pending_.buffer_ = buffer;
  }
}
void Surface::set_callback(wl_resource* resource) {
  this->pending_.callback_ = resource;
}

void Surface::listen_committed(util::Listener<std::nullptr_t*>& listener) {
  this->events_.committed_.add_listener(listener);
}

void Surface::commit() {
  if (this->pending_.buffer_.has_value()) {
    if (this->texture_.has_data()) {
      this->texture_.reset();
    }
    wl_shm_buffer* shm_buffer =
        wl_shm_buffer_get(this->pending_.buffer_.value());
    auto format = wl_shm_buffer_get_format(shm_buffer);
    if (format == WL_SHM_FORMAT_ARGB8888 || format == WL_SHM_FORMAT_XRGB8888) {
      this->texture_.from_wl_shm_buffer(shm_buffer);
    } else {
      LOG_ERR("yaza does not support surface buffer format (%u)", format);
    }
    this->pending_.buffer_ = std::nullopt;
  }

  if (this->pending_.callback_.has_value()) {
    auto* callback = this->pending_.callback_.value();
    wl_callback_send_done(callback, util::now_msec());
    wl_resource_destroy(callback);
    this->pending_.callback_ = std::nullopt;
  }

  this->events_.committed_.emit(nullptr);
}

namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void attach(wl_client* /*client*/, wl_resource* resource,
    wl_resource* buffer_resource, int32_t x, int32_t y) {
  auto* surface = static_cast<Surface*>(wl_resource_get_user_data(resource));
  surface->attach(buffer_resource, x, y);
}
void damage(wl_client* /*client*/, wl_resource* /*resource*/, int32_t /*x*/,
    int32_t /*y*/, int32_t /*width*/, int32_t /*height*/) {
  // TODO
}
void frame(wl_client* client, wl_resource* resource, uint32_t callback) {
  wl_resource* callback_resource =
      wl_resource_create(client, &wl_callback_interface, 1, callback);
  if (callback_resource) {
    static_cast<Surface*>(wl_resource_get_user_data(resource))
        ->set_callback(callback_resource);
  } else {
    wl_client_post_no_memory(client);
  }
}
void set_opaque_region(wl_client* /*client*/, wl_resource* /*resource*/,
    wl_resource* /*region_resource*/) {
  // TODO
}
void set_input_region(wl_client* /*client*/, wl_resource* /*resource*/,
    wl_resource* /*region_resource*/) {
  // TODO
}
void commit(wl_client* /*client*/, wl_resource* resource) {
  static_cast<Surface*>(wl_resource_get_user_data(resource))->commit();
}
void set_buffer_transform(
    wl_client* /*client*/, wl_resource* /*resource*/, int32_t /*transform*/) {
  // TODO
}
void set_buffer_scale(
    wl_client* /*client*/, wl_resource* /*resource*/, int32_t /*scale*/) {
  // TODO
}
void damage_buffer(wl_client* /*client*/, wl_resource* /*resource*/,
    int32_t /*x*/, int32_t /*y*/, int32_t /*width*/, int32_t /*height*/) {
  // TODO
}
void offset(wl_client* /*client*/, wl_resource* /*resource*/, int32_t /*x*/,
    int32_t /*y*/) {
  // TODO
}
const struct wl_surface_interface kImpl = {
    .destroy              = destroy,
    .attach               = attach,
    .damage               = damage,
    .frame                = frame,
    .set_opaque_region    = set_opaque_region,
    .set_input_region     = set_input_region,
    .commit               = commit,
    .set_buffer_transform = set_buffer_transform,
    .set_buffer_scale     = set_buffer_scale,
    .damage_buffer        = damage_buffer,
    .offset               = offset,
};

void destroy(wl_resource* resource) {
  auto* surface = static_cast<Surface*>(wl_resource_get_user_data(resource));
  delete surface;
}
}  // namespace

void create(wl_client* client, int version, uint32_t id) {
  auto* surface_resource =
      wl_resource_create(client, &wl_surface_interface, version, id);
  if (surface_resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  auto* surface = new Surface(id);
  wl_resource_set_implementation(surface_resource, &kImpl, surface, destroy);
}
}  // namespace yaza::wayland::surface
