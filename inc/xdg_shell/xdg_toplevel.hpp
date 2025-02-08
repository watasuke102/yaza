#pragma once

#include <wayland-server-protocol.h>

#include "xdg_shell/xdg_surface.hpp"

namespace yaza::xdg_shell::xdg_toplevel {
class XdgTopLevel {
 public:
  DISABLE_MOVE_AND_COPY(XdgTopLevel);
  XdgTopLevel(wl_resource* resource, xdg_surface::XdgSurface* surface);
  ~XdgTopLevel() = default;

  void set_activated(bool activated);

  void listen_committed(util::Listener<std::nullptr_t*>& listener);

 private:
  xdg_surface::XdgSurface* xdg_surface_;
  wl_resource*             resource_;
};

XdgTopLevel* create(wl_client* client, int version, uint32_t id,
    xdg_surface::XdgSurface* surface);
}  // namespace yaza::xdg_shell::xdg_toplevel
