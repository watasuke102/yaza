#pragma once

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>
#include <wayland-util.h>

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_int2.hpp>
#include <memory>
#include <numbers>
#include <unordered_map>

#include "common.hpp"
#include "input/input_listen_server.hpp"
#include "remote/session.hpp"
#include "renderer.hpp"
#include "util/signal.hpp"
#include "util/weakable_unique_ptr.hpp"
#include "wayland/surface.hpp"

namespace yaza::input {
enum class FocusedSurfaceState : uint8_t {
  DEFAULT,
  MOVING,
};

class Seat {
 public:
  DISABLE_MOVE_AND_COPY(Seat);
  Seat();
  ~Seat() = default;

  std::unordered_map<wl_client*, wl_resource* /*wl_pointer*/> pointer_resources;

  void set_surface_as_cursor(
      wl_resource* surface_resource, int32_t hotspot_x, int32_t hotspot_y);

  void handle_mouse_button(uint32_t button, wl_pointer_button_state state);
  void handle_mouse_wheel(float amount);
  void request_start_move(wl_client* client);
  void move_rel_pointing(float polar, float azimuthal);

 private:
  void check_surface_intersection();

  FocusedSurfaceState surface_state_ = FocusedSurfaceState::DEFAULT;
  util::WeakPtr<wayland::surface::Surface> focused_surface_;
  void set_focused_surface(util::WeakPtr<wayland::surface::Surface> surface,
      wl_resource* wl_pointer, wl_fixed_t x, wl_fixed_t y);
  void try_leave_focused_surface();

  util::WeakPtr<wayland::surface::Surface> cursor_;
  glm::ivec2                               hotspot_;
  void                                     move_cursor();
  float cursor_distance_ = Seat::kDefaultRayLen;

  // TODO: create class
  const glm::vec3        kBaseDirection = glm::vec3(0.F, 0.F, 1.F);
  const glm::vec3        kOrigin        = glm::vec3(0.F, 0.849F, -0.001F);
  constexpr static float kDefaultRayLen = 1.4F;
  struct {
    float     polar     = std::numbers::pi / 2.F;  // [0, pi]
    float     azimuthal = std::numbers::pi;        // x-z plane, [0, 2pi]
    glm::quat rot;
  } ray_;
  std::unique_ptr<Renderer> ray_renderer_;
  void                      init_ray_renderer();
  void                      update_ray_rot();

  std::unique_ptr<InputListenServer> input_listen_server_;

  util::Listener<remote::Session*> session_established_listener_;
  util::Listener<std::nullptr_t*>  session_disconnected_listener_;
};

}  // namespace yaza::input
