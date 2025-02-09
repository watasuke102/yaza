#pragma once

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>
#include <wayland-util.h>

#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <unordered_map>

#include "common.hpp"
#include "input/bounded_object.hpp"
#include "input/input_listen_server.hpp"
#include "input/ray_caster.hpp"
#include "util/weakable_unique_ptr.hpp"
#include "wayland/data_device/data_device.hpp"
#include "wayland/seat.hpp"

namespace yaza::input {
enum class FocusedObjState : uint8_t {
  DEFAULT,
  MOVING,
};

class ServerSeat {
 public:
  DISABLE_MOVE_AND_COPY(ServerSeat);
  ServerSeat()  = default;
  ~ServerSeat() = default;

  std::unordered_map<wl_client*, wayland::seat::ClientSeat*> client_seats;

  bool                              is_focused_client(wl_client* client);
  wayland::data_device::DataDevice* current_selection = nullptr;

  void set_surface_as_cursor(
      wl_resource* surface_resource, int32_t hotspot_x, int32_t hotspot_y);
  void set_keyboard_focused_surface(util::WeakPtr<input::BoundedObject> obj);

  void handle_mouse_button(uint32_t button, wl_pointer_button_state state);
  void handle_mouse_wheel(float amount);
  void request_start_move(wl_client* client);
  void move_rel_pointing(float polar, float azimuthal);

 private:
  void check_intersection();

  FocusedObjState                     obj_state_ = FocusedObjState::DEFAULT;
  util::WeakPtr<input::BoundedObject> focused_obj_;
  /// return true if `focused_obj_` is changed
  bool set_focused_obj(util::WeakPtr<input::BoundedObject> obj);

  util::WeakPtr<input::BoundedObject> keyboard_focused_surface_;
  void                                try_leave_keyboard();

  util::WeakPtr<input::BoundedObject> cursor_;
  glm::vec3 hotspot_;  // OpenGL scale (divided by kPixelPerMeter)
  float     cursor_distance_;
  void      move_cursor();

  InputListenServer input_listen_server_;
  RayCaster         ray_;
};

}  // namespace yaza::input
