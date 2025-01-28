#include "zwin/region.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>
#include <wayland-util.h>
#include <zen-remote/server/virtual-object.h>
#include <zwin-protocol.h>

#include <cstdint>

namespace yaza::zwin::region {
namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void add_cuboid(struct wl_client* /*client*/, struct wl_resource* /*resource*/,
    struct wl_array* /*half_size*/, struct wl_array* /*center*/,
    struct wl_array* /*quaternion*/) {
}
void add_sphere(struct wl_client* /*client*/, struct wl_resource* /*resource*/,
    struct wl_array* /*center*/, struct wl_array* /*radius*/) {
}
constexpr struct zwn_region_interface kImpl = {
    .destroy    = destroy,
    .add_cuboid = add_cuboid,
    .add_sphere = add_sphere,
};
}  // namespace

void create(wl_client* client, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &zwn_region_interface, 1, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yaza::zwin::region
