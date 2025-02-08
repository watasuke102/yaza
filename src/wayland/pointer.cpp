#include "wayland/pointer.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include "common.hpp"
#include "server.hpp"

namespace yaza::wayland::pointer {
namespace {
void set_cursor(wl_client* /*client*/, wl_resource* /*resource*/,
    uint32_t /*serial*/, wl_resource* surface_resource, int32_t hotspot_x,
    int32_t hotspot_y) {
  server::get().seat->set_surface_as_cursor(
      surface_resource, hotspot_x, hotspot_y);
}
void release(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
constexpr struct wl_pointer_interface kImpl = {
    .set_cursor = set_cursor,
    .release    = release,
};

void destroy(wl_resource* resource) {
  wl_list_remove(wl_resource_get_link(resource));
}
}  // namespace

void create(wl_client* client, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_pointer_interface, wl_pointer_interface.version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, destroy);
  server::get().seat->client_seats[client]->add_pointer(resource);
  LOG_DEBUG("created: wl_pointer@%d for client %p", id, (void*)client);
}
}  // namespace yaza::wayland::pointer
