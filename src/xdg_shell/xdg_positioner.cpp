#include "xdg_shell/xdg_positioner.hpp"

#include <wayland-server-core.h>
#include <xdg-shell-protocol.h>

namespace yaza::xdg_shell::xdg_positioner {
namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void set_size(wl_client* /*client*/, wl_resource* /*resource*/,
    int32_t /*width*/, int32_t /*height*/) {
  // TODO
}
void set_anchor_rect(wl_client* /*client*/, wl_resource* /*resource*/,
    int32_t /*x*/, int32_t /*y*/, int32_t /*width*/, int32_t /*height*/) {
  // TODO
}
void set_anchor(
    wl_client* /*client*/, wl_resource* /*resource*/, uint32_t /*anchor*/) {
  // TODO
}
void set_gravity(
    wl_client* /*client*/, wl_resource* /*resource*/, uint32_t /*gravity*/) {
  // TODO
}
void set_constraint_adjustment(wl_client* /*client*/, wl_resource* /*resource*/,
    uint32_t /*constraint_adjustment*/) {
  // TODO
}
void set_offset(wl_client* /*client*/, wl_resource* /*resource*/, int32_t /*x*/,
    int32_t /*y*/) {
  // TODO
}
void set_reactive(wl_client* /*client*/, wl_resource* /*resource*/) {
  // TODO
}
void set_parent_size(wl_client* /*client*/, wl_resource* /*resource*/,
    int32_t /*parent_width*/, int32_t /*parent_height*/) {
  // TODO
}
void set_parent_configure(
    wl_client* /*client*/, wl_resource* /*resource*/, uint32_t /*serial*/) {
  // TODO
}
constexpr struct xdg_positioner_interface kImpl = {
    .destroy                   = destroy,
    .set_size                  = set_size,
    .set_anchor_rect           = set_anchor_rect,
    .set_anchor                = set_anchor,
    .set_gravity               = set_gravity,
    .set_constraint_adjustment = set_constraint_adjustment,
    .set_offset                = set_offset,
    .set_reactive              = set_reactive,
    .set_parent_size           = set_parent_size,
    .set_parent_configure      = set_parent_configure,
};
}  // namespace

void create(wl_client* client, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &xdg_positioner_interface, xdg_positioner_interface.version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yaza::xdg_shell::xdg_positioner
