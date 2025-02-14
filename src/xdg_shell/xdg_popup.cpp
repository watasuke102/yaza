#include "xdg_shell/xdg_popup.hpp"

#include <wayland-server-core.h>
#include <xdg-shell-protocol.h>

#include <glm/ext/vector_float3.hpp>

#include "common.hpp"
#include "wayland/surface.hpp"
#include "xdg_shell/xdg_positioner.hpp"
#include "xdg_shell/xdg_surface.hpp"

namespace yaza::xdg_shell::xdg_popup {
XdgPopup::XdgPopup(wl_resource* resource, xdg_surface::XdgSurface* surface,
    xdg_surface::XdgSurface* parent, xdg_positioner::PositionerGeometry geom)
    : geom_(geom), parent_(parent), xdg_surface_(surface), resource_(resource) {
}
void XdgPopup::set_geom(xdg_positioner::PositionerGeometry geom) {
  this->geom_ = geom;

  auto parent_geom = this->parent_->get_wl_surface_geom();
  auto parent_half_pixel_size =
      this->parent_->get_wl_surface_texture_pixel_size() / 2;
  LOG_WARN("[popup] parent: %f, %f, %f", parent_geom.x(), parent_geom.y(),
      parent_geom.z());

  auto parent_left_top =
      parent_geom.pos() -
      (glm::vec3(parent_half_pixel_size.x + (geom.width / 2),
           -parent_half_pixel_size.y - (geom.height / 2), 0.F) /
          wayland::surface::kPixelPerMeter);
  //  (glm::vec3(parent_half_pixel_size.x,
  //              -parent_half_pixel_size.y, 0.F) /
  //             wayland::surface::kPixelPerMeter);
  LOG_WARN("[popup] L&R: %f, %f, %f", parent_left_top.x, parent_left_top.y,
      parent_left_top.z);
  auto popup_left_top =
      parent_left_top + 0.F * (glm::vec3(this->geom_.x, -this->geom_.y, 180.F) /
                                  wayland::surface::kPixelPerMeter);
  LOG_WARN("[popup] popup : %f, %f, %f", popup_left_top.x, popup_left_top.y,
      popup_left_top.z);

  this->xdg_surface_->move_wl_surface(popup_left_top, parent_geom.rot());
}
void XdgPopup::send_repositioned(uint32_t token) {
  xdg_popup_send_repositioned(this->resource_, token);
  this->xdg_surface_->send_configure();
}

namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void grab(wl_client* /*client*/, wl_resource* /*resource*/,
    wl_resource* /*seat*/, uint32_t /*serial*/) {
  // TODO
}
void reposition(wl_client* /*client*/, wl_resource* resource,
    wl_resource* positioner, uint32_t token) {
  auto* self = get(resource);
  self->set_geom(xdg_positioner::get(positioner)->geometry());
  self->send_repositioned(token);
}
constexpr struct xdg_popup_interface kImpl = {
    .destroy    = destroy,
    .grab       = grab,
    .reposition = reposition,
};

void destroy(wl_resource* resource) {
  delete get(resource);
}
}  // namespace

XdgPopup* create(wl_client* client, uint32_t id,
    xdg_surface::XdgSurface* popup_surface, xdg_surface::XdgSurface* parent,
    xdg_positioner::PositionerGeometry geom) {
  wl_resource* resource = wl_resource_create(
      client, &xdg_popup_interface, xdg_popup_interface.version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return nullptr;
  }
  auto* self = new XdgPopup(resource, popup_surface, parent, geom);
  self->set_geom(geom);
  wl_resource_set_implementation(resource, &kImpl, self, destroy);
  return self;
}
XdgPopup* get(wl_resource* resource) {
  assert(wl_resource_instance_of(resource, &xdg_popup_interface, &kImpl));
  return static_cast<XdgPopup*>(wl_resource_get_user_data(resource));
}
}  // namespace yaza::xdg_shell::xdg_popup
