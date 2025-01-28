#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include <cstdint>

#include "common.hpp"
#include "wayland/region.hpp"
#include "wayland/surface.hpp"

namespace yaza::wayland::compositor {
namespace {
void create_surface(wl_client* client, wl_resource* resource, uint32_t id) {
  LOG_DEBUG("create surface from client %p (id=%u)", (void*)client, id);
  surface::create(client, wl_resource_get_version(resource), id);
}

void create_region(wl_client* client, wl_resource* /*resource*/, uint32_t id) {
  LOG_DEBUG("create region from client %p (id=%u)", (void*)client, id);
  region::create(client, id);
}

constexpr struct wl_compositor_interface kImpl = {
    .create_surface = create_surface,
    .create_region  = create_region,
};
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_compositor_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yaza::wayland::compositor
