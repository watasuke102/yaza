#include "zwin/ray.hpp"

#include <wayland-server.h>
#include <zwin-protocol.h>

#include "common.hpp"
#include "server.hpp"

namespace yaza::zwin::ray {
namespace {
void release(wl_client* /*client*/, wl_resource* resource) {
  auto& rays = server::get().seat->ray_resources;
  for (auto it = rays.begin(); it != rays.end();) {
    if (it->second == resource) {
      it = rays.erase(it);
    } else {
      ++it;
    }
  }
  wl_resource_destroy(resource);
}
constexpr struct zwn_ray_interface kImpl = {
    .release = release,
};
}  // namespace

void create(wl_client* client, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &zwn_ray_interface, zwn_ray_interface.version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
  server::get().seat->ray_resources.emplace(client, resource);
  LOG_DEBUG("created: zwn_ray@%d for client %p", id, (void*)client);
}
}  // namespace yaza::zwin::ray
