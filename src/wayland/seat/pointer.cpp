#include "wayland/seat/pointer.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include "common.hpp"

namespace yaza::wayland::seat::pointer {
namespace {
void set_cursor(wl_client* /*client*/, wl_resource* /*resource*/,
    uint32_t /*serial*/, wl_resource* /*surface*/, int32_t /*hotspot_x*/,
    int32_t /*hotspot_y*/) {
  LOG_WARN("set_cursor");
  // TODO
}
void release(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
const struct wl_pointer_interface kImpl = {
    .set_cursor = set_cursor,
    .release    = release,
};
}  // namespace

void create(wl_client* client, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_pointer_interface, wl_pointer_interface.version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
  LOG_DEBUG("created: wl_pointer@%d", id);
}
}  // namespace yaza::wayland::seat::pointer
