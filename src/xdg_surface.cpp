#include "xdg_surface.hpp"

#include <wayland-server-protocol.h>

#include "common.hpp"
#include "xdg-shell-protocol.h"
#include "xdg_toplevel.hpp"

namespace yaza::xdg_surface {
namespace {
void destroy(wl_client* /* client */, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void get_toplevel(wl_client* client, wl_resource* resource, uint32_t id) {
  xdg_toplevel::new_xdg_toplevel(client, wl_resource_get_version(resource), id);
}
void get_popup(wl_client* /* client */, wl_resource* /* resource */,
    uint32_t /* id */, wl_resource* /* parent */,
    wl_resource* /* positioner */) {
  // TODO
}
void set_window_geometry(wl_client* /* client */, wl_resource* /* resource */,
    int32_t /* x */, int32_t /* y */, int32_t /* width */,
    int32_t /* height */) {
  // TODO
}
void ack_configure(wl_client* /* client */, wl_resource* /* resource */,
    uint32_t /* serial */) {
  // TODO
}
const struct xdg_surface_interface kImpl = {
    .destroy             = destroy,
    .get_toplevel        = get_toplevel,
    .get_popup           = get_popup,
    .set_window_geometry = set_window_geometry,
    .ack_configure       = ack_configure,
};
}  // namespace

void new_xdg_surface(wl_client* client, int version, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &xdg_surface_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
  LOG_DEBUG("xdg surface is created (client: %p, id: %u)", (void*)client, id);
}
}  // namespace yaza::xdg_surface
