#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include <cstdint>

#include "common.hpp"
#include "region.hpp"
#include "server.hpp"
#include "surface.hpp"

namespace yaza::compositor {
namespace {
void create_surface(
    struct wl_client* client, struct wl_resource* resource, uint32_t id) {
  LOG_DEBUG("create surface from client %p (id=%u)", (void*)client, id);
  surface::new_surface(client, resource, id);
}

void create_region(
    struct wl_client* client, struct wl_resource* /*resource*/, uint32_t id) {
  LOG_DEBUG("create region from client %p (id=%u)", (void*)client, id);
  region::new_region(client, id);
}

const struct wl_compositor_interface kImpl = {
    .create_surface = create_surface,
    .create_region  = create_region,
};
}  // namespace

void bind(wl_client* client, void* data, uint32_t version, uint32_t id) {
  auto* server = static_cast<yaza::Server*>(data);

  wl_resource* resource = wl_resource_create(
      client, &wl_compositor_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, server, nullptr);
}
}  // namespace yaza::compositor
