#include "xdg_shell/xdg_positioner.hpp"

#include <wayland-server-core.h>
#include <wayland-server.h>
#include <xdg-shell-protocol.h>

#include <cassert>
#include <memory>

#include "common.hpp"

namespace yaza::xdg_shell::xdg_positioner {
struct PositionInfo {
  PositionerGeometry     geom;
  PositionerGeometry     anchor_geom;
  xdg_positioner_anchor  anchor;
  xdg_positioner_gravity gravity;
};
XdgPositioner::XdgPositioner(wl_resource* resource)
    : info(std::make_unique<PositionInfo>()), resource_(resource) {
}
PositionerGeometry XdgPositioner::geometry() const {
  auto geom = PositionerGeometry{
      .x      = this->info->geom.x + this->info->anchor_geom.x,
      .y      = this->info->geom.y + this->info->anchor_geom.y,
      .width  = this->info->geom.width,
      .height = this->info->geom.height,
  };

  switch (this->info->anchor) {
    // left
    case XDG_POSITIONER_ANCHOR_TOP_LEFT:
    case XDG_POSITIONER_ANCHOR_LEFT:
    case XDG_POSITIONER_ANCHOR_BOTTOM_LEFT:
      break;
    // right
    case XDG_POSITIONER_ANCHOR_TOP_RIGHT:
    case XDG_POSITIONER_ANCHOR_RIGHT:
    case XDG_POSITIONER_ANCHOR_BOTTOM_RIGHT:
      geom.x += this->info->anchor_geom.width;
      break;
    // center
    default:
      geom.x += this->info->anchor_geom.width / 2;
      break;
  }
  switch (this->info->anchor) {
    // top
    case XDG_POSITIONER_ANCHOR_TOP:
    case XDG_POSITIONER_ANCHOR_TOP_LEFT:
    case XDG_POSITIONER_ANCHOR_TOP_RIGHT:
      break;
    // bottom
    case XDG_POSITIONER_ANCHOR_BOTTOM:
    case XDG_POSITIONER_ANCHOR_BOTTOM_LEFT:
    case XDG_POSITIONER_ANCHOR_BOTTOM_RIGHT:
      geom.y += this->info->anchor_geom.height;
      break;
    // middle
    default:
      geom.y += this->info->anchor_geom.height / 2;
      break;
  }

  switch (this->info->gravity) {
    // left
    case XDG_POSITIONER_GRAVITY_TOP_LEFT:
    case XDG_POSITIONER_GRAVITY_LEFT:
    case XDG_POSITIONER_GRAVITY_BOTTOM_LEFT:
      geom.x -= geom.width;
      break;
    // right
    case XDG_POSITIONER_GRAVITY_TOP_RIGHT:
    case XDG_POSITIONER_GRAVITY_RIGHT:
    case XDG_POSITIONER_GRAVITY_BOTTOM_RIGHT:
      break;
    // center
    default:
      geom.x -= geom.width / 2;
      break;
  }
  switch (this->info->gravity) {
    // top
    case XDG_POSITIONER_GRAVITY_TOP:
    case XDG_POSITIONER_GRAVITY_TOP_LEFT:
    case XDG_POSITIONER_GRAVITY_TOP_RIGHT:
      geom.y -= geom.height;
      break;
    // bottom
    case XDG_POSITIONER_GRAVITY_BOTTOM:
    case XDG_POSITIONER_GRAVITY_BOTTOM_LEFT:
    case XDG_POSITIONER_GRAVITY_BOTTOM_RIGHT:
      break;
    // middle
    default:
      geom.y -= geom.height / 2;
      break;
  }

  LOG_WARN("calcurated pos: %d, %d", geom.x, geom.y);
  return geom;
}

namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void set_size(wl_client* /*client*/, wl_resource* resource, int32_t width,
    int32_t height) {
  if (width <= 0) {
    wl_resource_post_error(
        resource, XDG_POSITIONER_ERROR_INVALID_INPUT, "invalid: width <= 0");
    return;
  }
  if (height < 0) {
    wl_resource_post_error(
        resource, XDG_POSITIONER_ERROR_INVALID_INPUT, "invalid: height <= 0");
    return;
  }
  auto* self              = get(resource);
  self->info->geom.width  = width;
  self->info->geom.height = height;
}
void set_anchor_rect(wl_client* /*client*/, wl_resource* resource, int32_t x,
    int32_t y, int32_t width, int32_t height) {
  if (width < 0) {
    wl_resource_post_error(
        resource, XDG_POSITIONER_ERROR_INVALID_INPUT, "width is negative");
    return;
  }
  if (height < 0) {
    wl_resource_post_error(
        resource, XDG_POSITIONER_ERROR_INVALID_INPUT, "height is negative");
    return;
  }
  auto* self                     = get(resource);
  self->info->anchor_geom.x      = x;
  self->info->anchor_geom.y      = y;
  self->info->anchor_geom.width  = width;
  self->info->anchor_geom.height = height;
}
void set_anchor(wl_client* /*client*/, wl_resource* resource, uint32_t anchor) {
  if (!xdg_positioner_anchor_is_valid(
          anchor, wl_resource_get_version(resource))) {
    wl_resource_post_error(
        resource, XDG_POSITIONER_ERROR_INVALID_INPUT, "invalid: anchor");
    return;
  }
  auto* self         = get(resource);
  self->info->anchor = static_cast<xdg_positioner_anchor>(anchor);
}
void set_gravity(
    wl_client* /*client*/, wl_resource* resource, uint32_t gravity) {
  if (!xdg_positioner_gravity_is_valid(
          gravity, wl_resource_get_version(resource))) {
    wl_resource_post_error(
        resource, XDG_POSITIONER_ERROR_INVALID_INPUT, "invalid: gravity");
    return;
  }
  auto* self          = get(resource);
  self->info->gravity = static_cast<xdg_positioner_gravity>(gravity);
}
void set_constraint_adjustment(wl_client* /*client*/, wl_resource* /*resource*/,
    uint32_t /*constraint_adjustment*/) {
}
void set_offset(
    wl_client* /*client*/, wl_resource* resource, int32_t x, int32_t y) {
  auto* self         = get(resource);
  self->info->geom.x = x;
  self->info->geom.y = y;
}
void set_reactive(wl_client* /*client*/, wl_resource* /*resource*/) {
}
void set_parent_size(wl_client* /*client*/, wl_resource* /*resource*/,
    int32_t /*parent_width*/, int32_t /*parent_height*/) {
}
void set_parent_configure(
    wl_client* /*client*/, wl_resource* /*resource*/, uint32_t /*serial*/) {
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

void destroy(wl_resource* resource) {
  delete get(resource);
}
}  // namespace

void create(wl_client* client, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &xdg_positioner_interface, xdg_positioner_interface.version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  auto* self = new XdgPositioner(resource);
  wl_resource_set_implementation(resource, &kImpl, self, destroy);
}
XdgPositioner* get(wl_resource* resource) {
  assert(wl_resource_instance_of(resource, &xdg_positioner_interface, &kImpl));
  return static_cast<XdgPositioner*>(wl_resource_get_user_data(resource));
}
}  // namespace yaza::xdg_shell::xdg_positioner
