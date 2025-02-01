#include "wayland/seat/seat.hpp"

#include <GLES3/gl32.h>
#include <linux/input-event-codes.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>
#include <wayland-util.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <memory>
#include <optional>
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
  uniform mat4 zVP;
  uniform mat4 local_model;
  layout(location = 0) in vec3 pos_in;
  out vec3 pos;

  void main() {
    gl_Position = zVP * local_model * vec4(pos_in, 1.0);
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
Seat::Seat() {
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

void Seat::set_surface_as_cursor(
    wl_resource* surface_resource, int32_t hotspot_x, int32_t hotspot_y) {
  this->hotspot_.x = hotspot_x;
  this->hotspot_.y = hotspot_y;

  if (this->cursor_.lock()) {
    if (this->cursor_->resource() == surface_resource) {
      return;
    }
    server::get().surfaces.push_front(std::move(this->cursor_));
  }
  assert(this->cursor_.lock() == nullptr);

  server::get().remove_expired_surfaces();
  auto& surfaces = server::get().surfaces;
  auto  it       = std::find_if(surfaces.begin(), surfaces.end(),
             [surface_resource](util::WeakPtr<surface::Surface>& s) {
        return s->resource() == surface_resource;
      });

  if (it != surfaces.end()) {
    this->cursor_ = *it;
    surfaces.erase(it);
    assert(this->cursor_.lock() != nullptr);
    this->cursor_->set_role(surface::Role::CURSOR);
    this->move_cursor();
  }
}
void Seat::move_cursor() {
  const auto r = this->cursor_distance_;  // shortening
  const auto pos =
      this->kOrigin +
      glm::vec3{                                                  //
          r * sin(this->ray_.polar) * sin(this->ray_.azimuthal),  //
          r * cos(this->ray_.polar),                              //
          r * sin(this->ray_.polar) * cos(this->ray_.azimuthal)};
  this->cursor_->move(pos, this->ray_.rot, this->hotspot_);
}

void Seat::handle_mouse_button(uint32_t button, wl_pointer_button_state state) {
  if (this->surface_state_ == FocusedSurfaceState::MOVING &&
      state == WL_POINTER_BUTTON_STATE_RELEASED) {
    this->surface_state_ = FocusedSurfaceState::DEFAULT;
    this->focused_surface_.reset();
    check_surface_intersection();
    return;
  }

  auto* surface = this->focused_surface_.lock();
  if (!surface) {
    return;
  }
  if (auto* wl_pointer = this->pointer_resources[surface->client()]) {
    wl_pointer_send_button(wl_pointer, server::get().next_serial(),
        util::now_msec(), button, state);
    wl_pointer_send_frame(wl_pointer);
  }
}
void Seat::handle_mouse_wheel(float amount) {
  auto* surface = this->focused_surface_.lock();
  if (!surface) {
    return;
  }
  if (auto* wl_pointer = this->pointer_resources[surface->client()]) {
    wl_pointer_send_axis(wl_pointer, util::now_msec(),
        WL_POINTER_AXIS_VERTICAL_SCROLL, wl_fixed_from_double(amount));
    wl_pointer_send_frame(wl_pointer);
  }
}

void Seat::request_start_move(wl_client* client) {
  auto* surface = this->focused_surface_.lock();
  if (!surface) {
    return;
  }
  if (surface->client() != client) {
    return;
  }
  if (auto* wl_pointer = this->pointer_resources[surface->client()]) {
    wl_pointer_send_leave(
        wl_pointer, server::get().next_serial(), surface->resource());
    wl_pointer_send_frame(wl_pointer);
  }
  this->surface_state_ = FocusedSurfaceState::MOVING;
}

void Seat::move_rel_pointing(float polar, float azimuthal) {
  constexpr float kPolarMin  = std::numbers::pi / 8.F;
  constexpr float kPolarMax  = std::numbers::pi - kPolarMin;
  auto            diff_polar = this->ray_.polar;
  this->ray_.polar = std::clamp(this->ray_.polar + polar, kPolarMin, kPolarMax);
  diff_polar       = this->ray_.polar - diff_polar;
  this->ray_.azimuthal += azimuthal;
  this->update_ray_rot();
  if (this->ray_renderer_) {
    this->ray_renderer_->commit();
  }

  switch (this->surface_state_) {
    case FocusedSurfaceState::DEFAULT:
      this->check_surface_intersection();
      break;
    case FocusedSurfaceState::MOVING:
      if (auto* surface = this->focused_surface_.lock()) {
        surface->move(diff_polar, azimuthal);
      }
      break;
  }

  if (this->cursor_.lock()) {
    this->move_cursor();
  }
}
void Seat::check_surface_intersection() {
  const auto direction = glm::rotate(this->ray_.rot, this->kBaseDirection);

  util::WeakPtr<surface::Surface>              nearest_surface;
  std::optional<surface::SurfaceIntersectInfo> nearest_surface_info =
      std::nullopt;
  auto& surfaces = server::get().surfaces;
  for (auto it = surfaces.begin(); it != surfaces.end();) {
    if (auto* surface = it->lock()) {
      auto result = surface->intersected_at(this->kOrigin, direction);
      if (!result.has_value()) {
        ++it;
        continue;
      }
      if (!nearest_surface_info.has_value() ||
          result->distance <= nearest_surface_info->distance) {
        nearest_surface      = *it;
        nearest_surface_info = result.value();
      }
      ++it;
    } else {
      it = surfaces.erase(it);
    }
  }

  if (nearest_surface_info.has_value()) {
    wl_resource* wl_pointer =
        this->pointer_resources[nearest_surface.lock()->client()];
    if (!wl_pointer) {
      return;
    }
    auto x = wl_fixed_from_double(nearest_surface_info->sx);
    auto y = wl_fixed_from_double(nearest_surface_info->sy);
    this->set_focused_surface(nearest_surface, wl_pointer, x, y);
    wl_pointer_send_motion(wl_pointer, util::now_msec(), x, y);
    wl_pointer_send_frame(wl_pointer);
    // show the cursor nearer to origin than any other surfaces
    this->cursor_distance_ = nearest_surface_info->distance * 0.98F;
  } else {
    // there is no intersected surface
    this->try_leave_focused_surface();
    this->focused_surface_.reset();
  }
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
asterisk pos is expressed by (polar, azimuthal) = (0, pi/2)
    plus pos is expressed by (polar, azimuthal) = (0, 0)
*/
void Seat::init_ray_renderer() {
  this->ray_renderer_ = std::make_unique<Renderer>(kVertShader, kFragShader);
  std::vector<float> vertices = {
      0.F,
      0.F,
      0.F,  //
      this->kBaseDirection.x,
      this->kBaseDirection.y,
      this->kBaseDirection.z,
  };
  this->ray_renderer_->register_buffer(0, 3, GL_FLOAT, vertices.data(),
      sizeof(float) * vertices.size());  // NOLINT
  this->ray_renderer_->request_draw_arrays(GL_LINE_STRIP, 0, 2);
  this->update_ray_rot();
  this->ray_renderer_->commit();
}
void Seat::update_ray_rot() {
  this->ray_.rot =
      glm::angleAxis(this->ray_.azimuthal, glm::vec3{0.F, 1.F, 0.F}) *
      glm::angleAxis((std::numbers::pi_v<float> / 2.F) - this->ray_.polar,
          glm::vec3{-1.F, 0.F, 0.F});

  if (this->ray_renderer_) {
    auto      length = this->cursor_.lock() ? this->cursor_distance_ * 0.75 :
                                              Seat::kDefaultRayLen;
    glm::mat4 mat    = glm::translate(glm::mat4(1.F), this->kOrigin) *
                    glm::toMat4(this->ray_.rot) *
                    glm::scale(glm::mat4(1.F), glm::vec3{length, 1.F, 1.F});
    this->ray_renderer_->set_uniform_matrix(0, "local_model", mat);
  }
}

namespace {
void get_pointer(wl_client* client, wl_resource* /*resource*/, uint32_t id) {
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
