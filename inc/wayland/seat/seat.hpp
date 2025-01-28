#pragma once

#include <wayland-server-core.h>
#include <wayland-server.h>
#include <wayland-util.h>

#include <glm/ext/vector_float3.hpp>
#include <memory>
#include <numbers>
#include <optional>
#include <unordered_map>

#include "common.hpp"
#include "remote/session.hpp"
#include "renderer.hpp"
#include "util/signal.hpp"
#include "util/weakable_unique_ptr.hpp"
#include "wayland/seat/input_listen_server.hpp"
#include "wayland/surface.hpp"

namespace yaza::wayland::seat {
class Seat {
 public:
  DISABLE_MOVE_AND_COPY(Seat);
  Seat();
  ~Seat() = default;

  std::unordered_map<wl_client*, wl_resource* /*wl_pointer*/> pointer_resources;

  void move_rel_pointing(float polar, float azimuthal);

 private:
  void check_surface_intersection();

  util::WeakPtr<surface::Surface> focused_surface_;
  void set_focused_surface(util::WeakPtr<surface::Surface> surface,
      wl_resource* wl_pointer, wl_fixed_t x, wl_fixed_t y);
  void try_leave_focused_surface();

  const glm::vec3 kOrigin = glm::vec3(0.F, 0.849F, -0.001F);
  struct {
    float polar     = std::numbers::pi / 2.F;  // [0, pi]
    float azimuthal = std::numbers::pi;        // x-z plane, [0, 2pi]
  } pointing_;
  std::vector<BufferElement> ray_vertices_;
  std::unique_ptr<Renderer>  ray_renderer_;
  void                       init_ray_renderer();
  void                       update_ray_vertices();

  std::unique_ptr<InputListenServer> input_listen_server_;

  util::Listener<remote::Session*> session_established_listener_;
  util::Listener<std::nullptr_t*>  session_disconnected_listener_;
};

void bind(wl_client* client, void* data, uint32_t version, uint32_t id);
}  // namespace yaza::wayland::seat
