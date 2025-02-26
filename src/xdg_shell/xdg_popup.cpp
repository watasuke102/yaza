#include "xdg_shell/xdg_popup.hpp"

#include <wayland-server-core.h>
#include <xdg-shell-protocol.h>

#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/gtx/string_cast.hpp>

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
  const auto geom_pixel_center_pos =
      glm::ivec2(geom.x + (geom.width / 2), geom.y + (geom.height / 2));

  const auto parent_half_pixel_size =
      this->parent_->get_wl_surface_texture_pixel_size() / 2;
  const auto center_based_gl_pos =
      glm::vec3(geom_pixel_center_pos.x - parent_half_pixel_size.x,
          -(geom_pixel_center_pos.y - parent_half_pixel_size.y), 0.F) /
      wayland::surface::kPixelPerMeter;

  auto       parent_geom = this->parent_->get_wl_surface_geom();
  const auto popup_pos   = parent_geom.pos() + center_based_gl_pos;

  LOG_INFO("popup(xdg_surface@%2d) moveto: %s",
      wl_resource_get_id(this->resource_), glm::to_string(popup_pos).c_str());
  this->xdg_surface_->move_wl_surface(popup_pos, parent_geom.rot());
  LOG_INFO(
      "popup(xdg_surface@%2d) move end", wl_resource_get_id(this->resource_));
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
