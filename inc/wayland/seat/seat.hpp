#pragma once

#include <wayland-server-core.h>

#include <memory>
#include <numbers>

#include "common.hpp"
#include "remote/session.hpp"
#include "renderer.hpp"
#include "util/signal.hpp"
#include "wayland/seat/input_listen_server.hpp"

namespace yaza::wayland::seat {
class Seat {
 public:
  DISABLE_MOVE_AND_COPY(Seat);
  Seat();
  ~Seat() = default;

  void move_rel_pointing(float polar, float azimuthal);

 private:
  struct {
    float polar_     = std::numbers::pi / 2.F;  // [0, pi]
    float azimuthal_ = std::numbers::pi;        // x-z plane, [0, 2pi]
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
