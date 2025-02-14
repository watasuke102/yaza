#pragma once

#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include <cstddef>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_int2.hpp>

#include "util/box.hpp"
#include "util/signal.hpp"
#include "util/weakable_unique_ptr.hpp"
#include "wayland/surface.hpp"

namespace yaza::xdg_shell::xdg_surface {
class XdgSurface {
 public:
  DISABLE_MOVE_AND_COPY(XdgSurface);
  XdgSurface(uint32_t id, wl_resource* resource,
      util::WeakPtr<wayland::surface::Surface>&& surface);
  ~XdgSurface();

  void send_configure();

  const util::Box& get_wl_surface_geom();
  glm::ivec2       get_wl_surface_texture_pixel_size();
  void             set_wl_surface_role(
                  wayland::surface::Role role, wayland::surface::RoleObject obj);
  void move_wl_surface(glm::vec3 pos, glm::quat rot);

 private:
  bool is_first_commit_ = true;

  util::WeakPtr<wayland::surface::Surface> wl_surface_;
  util::Listener<std::nullptr_t*>          wl_surface_committed_listener_;
  wl_resource*                             resource_;
  uint32_t                                 id_;
};

void create(wl_client* client, int version, uint32_t id,
    util::WeakPtr<wayland::surface::Surface>&& surface);
}  // namespace yaza::xdg_shell::xdg_surface
