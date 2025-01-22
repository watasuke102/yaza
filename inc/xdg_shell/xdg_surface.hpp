#pragma once

#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include <cstddef>

#include "util/signal.hpp"
#include "wayland/surface.hpp"

namespace yaza::xdg_shell::xdg_surface {
class XdgSurface {
 public:
  DISABLE_MOVE_AND_COPY(XdgSurface);
  XdgSurface(
      uint32_t id, wl_resource* resource, wayland::surface::Surface* surface);
  ~XdgSurface();

 private:
  util::Listener<std::nullptr_t*> wl_surface_committed_listener_;
  wl_resource*                    resource_;
  uint32_t                        id_;
};

void create(wl_client* client, int version, uint32_t id,
    wayland::surface::Surface* surface);
}  // namespace yaza::xdg_shell::xdg_surface
