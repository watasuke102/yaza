#pragma once

#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include <cstdint>

#include "common.hpp"
#include "wayland/surface.hpp"
#include "xdg_shell/xdg_positioner.hpp"
#include "xdg_shell/xdg_surface.hpp"

namespace yaza::xdg_shell::xdg_popup {
class XdgPopup {
 public:
  DISABLE_MOVE_AND_COPY(XdgPopup);
  XdgPopup(wl_resource* resource, xdg_surface::XdgSurface* popup_surface,
      xdg_surface::XdgSurface* parent, xdg_positioner::PositionerGeometry geom);
  ~XdgPopup() = default;

  void set_geom(xdg_positioner::PositionerGeometry geom);
  void send_repositioned(uint32_t token);

 private:
  xdg_positioner::PositionerGeometry geom_;

  xdg_surface::XdgSurface* parent_;
  xdg_surface::XdgSurface* xdg_surface_;
  wl_resource*             resource_;
};

XdgPopup* create(wl_client* client, uint32_t id,
    xdg_surface::XdgSurface* popup_surface, xdg_surface::XdgSurface* parent,
    xdg_positioner::PositionerGeometry geom);
XdgPopup* get(wl_resource* resource);
}  // namespace yaza::xdg_shell::xdg_popup
