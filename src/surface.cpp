#include "surface.hpp"

#include <exception>

#include "common.hpp"
#include "wayland-server-core.h"
#include "wayland-server-protocol.h"
#include "wayland-server.h"

namespace yaza::surface {
namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void attach(wl_client* /*client*/, wl_resource* /*resource*/,
    wl_resource* /*buffer_resource*/, int32_t /*x*/, int32_t /*y*/) {
}
void damage(wl_client* /*client*/, wl_resource* /*resource*/, int32_t /*x*/,
    int32_t /*y*/, int32_t /*width*/, int32_t /*height*/) {
  // TODO
}
void frame(wl_client* /*client*/, wl_resource* /*resource*/, uint32_t /*id*/) {
  // TODO
}
void set_opaque_region(wl_client* /*client*/, wl_resource* /*resource*/,
    wl_resource* /*region_resource*/) {
  // TODO
}
void set_input_region(wl_client* /*client*/, wl_resource* /*resource*/,
    wl_resource* /*region_resource*/) {
  // TODO
}
void commit(wl_client* /*client*/, wl_resource* /*resource*/) {
  // TODO
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

class Surface {
 public:
  DISABLE_MOVE_AND_COPY(Surface);
  Surface(wl_client* client, wl_resource* resource, uint32_t id) {
    this->resource_ = wl_resource_create(
        client, &wl_surface_interface, wl_resource_get_version(resource), id);
    if (this->resource_ == nullptr) {
      wl_resource_post_no_memory(resource);
      throw std::exception();
    }
  }
  ~Surface() {
  }
  inline wl_resource* resource() {
    return this->resource_;
  }

 private:
  wl_resource* resource_;
};

void delete_surface(wl_resource* resource) {
  auto* surface = static_cast<Surface*>(wl_resource_get_user_data(resource));
  delete surface;
}
}  // namespace

void new_surface(wl_client* client, wl_resource* resource, uint32_t id) {
  auto* surface = new Surface(client, resource, id);
  LOG_DEBUG("surface created (id: %u)", id);
  wl_resource_set_implementation(
      surface->resource(), &kImpl, surface, delete_surface);
}
}  // namespace yaza::surface
