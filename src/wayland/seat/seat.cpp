#include "wayland/seat/seat.hpp"

#include <GLES3/gl32.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include <cstdint>
#include <cstdlib>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <vector>

#include "common.hpp"
#include "remote/remote.hpp"
#include "renderer.hpp"
#include "wayland/seat/pointer.hpp"

namespace yaza::wayland::seat {
namespace {
// clang-format off
constexpr auto* kVertShader = GLSL(
  uniform mat4 zMVP;
  layout(location = 0) in vec3 pos;

  void main() {
    gl_Position = zMVP * vec4(pos, 1.0);
  }
);
constexpr auto* kFragShader = GLSL(
  out vec4 color;

  void main() {
    color = vec4(0.0, 1.0, 0.0, 1.0);
  }
);
// clang-format on
}  // namespace
Seat::Seat()
    : ray_vertices_{
          {.x_ = 0.F, .y_ = 0.F, .z_ = 0.F, .u_ = 0.F, .v_ = 0.F},
          {.x_ = 0.F, .y_ = 0.F, .z_ = 0.F, .u_ = 0.F, .v_ = 0.F},
} {
  if (remote::g_remote->has_session()) {
    this->init_ray_renderer();
  }
  this->session_established_listener_.set_handler(
      [this](remote::Session* /*session*/) {
        this->init_ray_renderer();
      });
  remote::g_remote->listen_session_established(
      this->session_established_listener_);

  this->session_disconnected_listener_.set_handler(
      [this](std::nullptr_t* /*data*/) {
        this->ray_renderer_ = nullptr;
      });
  remote::g_remote->listen_session_disconnected(
      this->session_disconnected_listener_);
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
  this->ray_renderer_->move_abs(0.F, 0.9F, -0.F);
  this->ray_renderer_->request_draw_arrays(GL_LINE_STRIP, 0, 2);
  this->update_ray_vertices();
  this->ray_renderer_->commit();
}
void Seat::update_ray_vertices() {
  assert(this->ray_renderer_ != nullptr);
  auto r = 1.5F;
  this->ray_vertices_[1].x_ =
      r * sin(this->pointing_.polar_) * sin(this->pointing_.azimuthal_),
  this->ray_vertices_[1].y_ = r * cos(this->pointing_.polar_),
  this->ray_vertices_[1].z_ =
      r * sin(this->pointing_.polar_) * cos(this->pointing_.azimuthal_);
  this->ray_renderer_->set_vertex(this->ray_vertices_);
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
const struct wl_seat_interface kImpl = {
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
