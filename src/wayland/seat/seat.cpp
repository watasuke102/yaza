#include "wayland/seat/seat.hpp"

#include <GLES3/gl32.h>
#include <linux/input-event-codes.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>
#include <wayland-util.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "common.hpp"
#include "renderer.hpp"
#include "server.hpp"
#include "util/time.hpp"
#include "util/weakable_unique_ptr.hpp"
#include "wayland/seat/input_listen_server.hpp"
#include "wayland/seat/pointer.hpp"
#include "wayland/surface.hpp"

namespace yaza::wayland::seat {
namespace {
// clang-format off
constexpr auto* kVertShader = GLSL(
  uniform mat4 zMVP;
  layout(location = 0) in vec3 pos_in;
  out vec3 pos;

  void main() {
    gl_Position = zMVP * vec4(pos_in, 1.0);
    pos = gl_Position.xyz;
  }
);
constexpr auto* kFragShader = GLSL(
  in  vec3 pos;
  out vec4 color;

  void main() {
    color = vec4(0.0, 1.0, 0.0, 1.0);
  }
);
// clang-format on
}  // namespace
Seat::Seat()
    : ray_vertices_{
          {.x = 0.F, .y = 0.F, .z = 0.F, .u = 0.F, .v = 0.F},
          {.x = 0.F, .y = 0.F, .z = 0.F, .u = 0.F, .v = 0.F},
} {
  if (server::get().remote->has_session()) {
    this->init_ray_renderer();
  }
  this->session_established_listener_.set_handler(
      [this](remote::Session* /*session*/) {
        this->init_ray_renderer();
      });
  server::get().remote->listen_session_established(
      this->session_established_listener_);

  this->session_disconnected_listener_.set_handler(
      [this](std::nullptr_t* /*data*/) {
        this->ray_renderer_ = nullptr;
      });
  server::get().remote->listen_session_disconnected(
      this->session_disconnected_listener_);
  this->input_listen_server_ = std::make_unique<InputListenServer>();
}

void Seat::mouse_button(wl_pointer_button_state state) {
  auto* surface = this->focused_surface_.lock();
  if (!surface) {
    return;
  }
  if (auto* wl_pointer = this->pointer_resources[surface->client()]) {
    wl_pointer_send_button(wl_pointer, server::get().next_serial(),
        util::now_msec(), BTN_LEFT, state);
  }
}

void Seat::move_rel_pointing(float polar, float azimuthal) {
  constexpr float kPolarMin = std::numbers::pi / 8.F;
  constexpr float kPolarMax = std::numbers::pi - kPolarMin;
  this->pointing_.polar =
      std::clamp(this->pointing_.polar + polar, kPolarMin, kPolarMax);
  this->pointing_.azimuthal += azimuthal;
  this->update_ray_vertices();
  if (this->ray_renderer_) {
    this->ray_renderer_->commit();
  }
  this->check_surface_intersection();
}
void Seat::check_surface_intersection() {
  auto direction = glm::vec3(this->ray_vertices_[1].x, this->ray_vertices_[1].y,
      this->ray_vertices_[1].z);
  auto& surfaces = server::get().surfaces;

  for (auto it = surfaces.begin(); it != surfaces.end();) {
    if (auto* surface = it->lock()) {
      util::WeakPtr<surface::Surface> surface_weakptr = *it;
      ++it;
      auto result = surface->intersected_at(this->kOrigin, direction);
      if (!result.has_value()) {
        continue;
      }
      wl_resource* wl_pointer = this->pointer_resources[surface->client()];
      if (!wl_pointer) {
        return;
      }
      auto x = wl_fixed_from_double(result->first);
      auto y = wl_fixed_from_double(result->second);
      this->set_focused_surface(surface_weakptr, wl_pointer, x, y);
      wl_pointer_send_motion(wl_pointer, util::now_msec(), x, y);
      return;
    } else {  // NOLINT(readability-else-after-return): ???
      it = surfaces.erase(it);
    }
  }
  // there is no intersected surface
  this->try_leave_focused_surface();
  this->focused_surface_.reset();
}

void Seat::set_focused_surface(util::WeakPtr<surface::Surface> surface,
    wl_resource* wl_pointer, wl_fixed_t x, wl_fixed_t y) {
  if (surface == this->focused_surface_) {
    return;
  }
  wl_pointer_send_enter(wl_pointer, server::get().next_serial(),
      surface.lock()->resource(), x, y);
  this->try_leave_focused_surface();
  this->focused_surface_.swap(surface);
}
void Seat::try_leave_focused_surface() {
  wayland::surface::Surface* surface = this->focused_surface_.lock();
  if (!surface) {
    return;
  }
  if (auto* wl_pointer = this->pointer_resources[surface->client()]) {
    wl_pointer_send_leave(
        wl_pointer, server::get().next_serial(), surface->resource());
  }
}

/*
         +y  -z (head facing direction)
          ^  /
          | /
          |/
----------------*----> +x
         /|
        + |
       /  |
     +z
asterisk pos is expressed by (polar, azimuthal) = (0, 0)
    plus pos is expressed by (polar, azimuthal) = (0, pi/2)
*/
void Seat::init_ray_renderer() {
  this->ray_renderer_ = std::make_unique<Renderer>(kVertShader, kFragShader);
  std::vector<BufferElement> v;
  this->ray_renderer_->move_abs(
      this->kOrigin.x, this->kOrigin.y, this->kOrigin.z);
  this->ray_renderer_->request_draw_arrays(GL_LINE_STRIP, 0, 2);
  this->update_ray_vertices();
  this->ray_renderer_->commit();
}
void Seat::update_ray_vertices() {
  auto r = 1.5F;
  this->ray_vertices_[1].x =
      r * sin(this->pointing_.polar) * sin(this->pointing_.azimuthal),
  this->ray_vertices_[1].y = r * cos(this->pointing_.polar),
  this->ray_vertices_[1].z =
      r * sin(this->pointing_.polar) * cos(this->pointing_.azimuthal);
  if (this->ray_renderer_) {
    this->ray_renderer_->set_vertex(this->ray_vertices_);
  }
}

namespace {
void get_pointer(wl_client* client, wl_resource* /*resource*/, uint32_t id) {
  LOG_WARN("get_pointer");
  pointer::create(client, id);
}
void get_keyboard(
    wl_client* /*client*/, wl_resource* /*resource*/, uint32_t /*id*/) {
  LOG_WARN("get_keyboard");
  // TODO
}
void get_touch(wl_client* client, wl_resource* /*resource*/, uint32_t /*id*/) {
  wl_client_post_implementation_error(
      client, "touch device is not implemented");
}
void release(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
constexpr struct wl_seat_interface kImpl = {
    .get_pointer  = get_pointer,
    .get_keyboard = get_keyboard,
    .get_touch    = get_touch,
    .release      = release,
};
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_seat_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
  wl_seat_send_capabilities(
      resource, WL_SEAT_CAPABILITY_POINTER | WL_SEAT_CAPABILITY_KEYBOARD);
}
}  // namespace yaza::wayland::seat
