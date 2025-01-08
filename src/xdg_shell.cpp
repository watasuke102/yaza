
#include "server.hpp"
#include "xdg-shell-protocol.h"
#include "xdg_surface.hpp"

namespace yaza::xdg_shell {
namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void create_positioner(
    wl_client* /*client*/, wl_resource* /*resource*/, uint32_t /*id*/) {
  // TODO
}
void get_xdg_surface(wl_client* client, wl_resource* resource, uint32_t id,
    wl_resource* /*surface_resource*/) {
  xdg_surface::new_xdg_surface(client, wl_resource_get_version(resource), id);
}
void pong(
    wl_client* /*client*/, wl_resource* /*resource*/, uint32_t /*serial*/) {
  // TODO
}
const struct xdg_wm_base_interface kXdgWmBaseImpl = {
    .destroy           = destroy,
    .create_positioner = create_positioner,
    .get_xdg_surface   = get_xdg_surface,
    .pong              = pong,
};
}  // namespace

void bind(wl_client* client, void* data, uint32_t version, uint32_t id) {
  auto* server = static_cast<yaza::Server*>(data);

  wl_resource* resource = wl_resource_create(
      client, &xdg_wm_base_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kXdgWmBaseImpl, server, nullptr);
}
}  // namespace yaza::xdg_shell
