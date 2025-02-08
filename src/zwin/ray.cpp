#include "zwin/ray.hpp"

#include <wayland-server.h>
#include <zwin-protocol.h>

#include "common.hpp"
#include "server.hpp"

namespace yaza::zwin::ray {
namespace {
void release(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
constexpr struct zwn_ray_interface kImpl = {
    .release = release,
};

void destroy(wl_resource* resource) {
  wl_list_remove(wl_resource_get_link(resource));
}
}  // namespace

void create(wl_client* client, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &zwn_ray_interface, zwn_ray_interface.version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, destroy);
  server::get().seat->client_seats[client]->add_ray(resource);
  LOG_DEBUG("created: zwn_ray@%d for client %p", id, (void*)client);
}
}  // namespace yaza::zwin::ray
