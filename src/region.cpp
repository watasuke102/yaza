#include "region.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>

namespace yaza::region {
namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void add(wl_client* /*client*/, wl_resource* /*resource*/, int32_t /*x*/,
    int32_t /*y*/, int32_t /*width*/, int32_t /*height*/) {
  // TODO
}
void subtract(wl_client* /*client*/, wl_resource* /*resource*/, int32_t /*x*/,
    int32_t /*y*/, int32_t /*width*/, int32_t /*height*/) {
  // TODO
}

const struct wl_region_interface kImpl = {
    .destroy  = destroy,
    .add      = add,
    .subtract = subtract,
};
}  // namespace

void new_region(wl_client* client, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &wl_region_interface, 1, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yaza::region
