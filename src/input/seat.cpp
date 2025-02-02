#include "input/seat.hpp"

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
#include "input/bounded_object.hpp"
#include "input/input_listen_server.hpp"
#include "renderer.hpp"
#include "server.hpp"
#include "util/weakable_unique_ptr.hpp"
#include "wayland/surface.hpp"

namespace yaza::input {
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

bool Seat::is_focused_client(wl_client* client) {
  if (auto* focused_obj = this->focused_obj_.lock()) {
    return client == focused_obj->client();
  }
  return false;
}

RayGeometry Seat::ray_geometry() {
  return {.origin = this->kOrigin,
      .direction  = glm::rotate(this->ray_.rot, this->kBaseDirection)};
}

void Seat::set_surface_as_cursor(
    wl_resource* surface_resource, int32_t hotspot_x, int32_t hotspot_y) {
  this->hotspot_.x = hotspot_x;
  this->hotspot_.y = hotspot_y;

  if (this->cursor_.lock()) {
    if (this->cursor_->resource() == surface_resource) {
      return;
    }
    {
      auto* cursor =
          dynamic_cast<wayland::surface::Surface*>(this->cursor_.lock());
      cursor->set_active(false);
    }
    server::get().surfaces.push_front(std::move(this->cursor_));
  }
  assert(this->cursor_.lock() == nullptr);

  server::get().remove_expired_surfaces();
  auto& surfaces = server::get().surfaces;
  auto  it       = std::find_if(surfaces.begin(), surfaces.end(),
             [surface_resource](util::WeakPtr<input::BoundedObject>& s) {
        return s->resource() == surface_resource;
      });

  if (it != surfaces.end()) {
    this->cursor_ = *it;
    surfaces.erase(it);
    assert(this->cursor_.lock() != nullptr);
    auto* cursor =
        dynamic_cast<wayland::surface::Surface*>(this->cursor_.lock());
    cursor->set_role(wayland::surface::Role::CURSOR);
    cursor->set_active(true);
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
  auto* cursor = dynamic_cast<wayland::surface::Surface*>(this->cursor_.lock());
  cursor->move(pos, this->ray_.rot, this->hotspot_);
}

void Seat::handle_mouse_button(uint32_t button, wl_pointer_button_state state) {
  if (this->obj_state_ == FocusedObjState::MOVING &&
      state == WL_POINTER_BUTTON_STATE_RELEASED) {
    this->obj_state_ = FocusedObjState::DEFAULT;
    this->focused_obj_.reset();
    check_intersection();
    return;
  }

  if (auto* obj = this->focused_obj_.lock()) {
    obj->button(button, state);
    obj->frame();
  }
}
void Seat::handle_mouse_wheel(float amount) {
  if (auto* obj = this->focused_obj_.lock()) {
    obj->axis(amount);
    obj->frame();
  }
}

void Seat::request_start_move(wl_client* client) {
  auto* obj = this->focused_obj_.lock();
  if (!obj) {
    return;
  }
  if (obj->client() != client || !obj->is_active()) {
    return;
  }
  obj->leave();
  obj->frame();
  this->obj_state_ = FocusedObjState::MOVING;
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

  switch (this->obj_state_) {
    case FocusedObjState::DEFAULT:
      this->check_intersection();
      break;
    case FocusedObjState::MOVING:
      if (auto* obj = this->focused_obj_.lock()) {
        obj->move(diff_polar, azimuthal);
      }
      break;
  }

  if (this->cursor_.lock()) {
    this->move_cursor();
  }
}
void Seat::check_intersection() {
  util::WeakPtr<BoundedObject> nearest_obj;
  std::optional<IntersectInfo> nearest_obj_info = std::nullopt;

  auto check =
      [this, direction = glm::rotate(this->ray_.rot, this->kBaseDirection),
          &nearest_obj, &nearest_obj_info](auto&& objs) {
        for (auto it = objs.begin(); it != objs.end();) {
          if (auto* surface = it->lock()) {
            auto result = surface->check_intersection(this->kOrigin, direction);
            if (!result.has_value()) {
              ++it;
              continue;
            }
            if (!nearest_obj_info.has_value() ||
                result->distance <= nearest_obj_info->distance) {
              nearest_obj      = *it;
              nearest_obj_info = result.value();
            }
            ++it;
          } else {
            it = objs.erase(it);
          }
        }
      };
  check(server::get().surfaces);
  check(server::get().bounded_apps);

  if (nearest_obj_info.has_value()) {
    if (!nearest_obj->is_active()) {
      LOG_WARN("intersected obj is inactive (distance: %f)",
          nearest_obj_info->distance);
      return;  // FIXME: is this error handling correct?
    }
    bool focus_changed = this->set_focused_obj(nearest_obj);
    if (focus_changed) {
      this->focused_obj_->enter(nearest_obj_info.value());
    }
    this->focused_obj_->motion(nearest_obj_info.value());
    this->focused_obj_->frame();
    // show the cursor nearer to origin than hit position
    this->cursor_distance_ = nearest_obj_info->distance * 0.98F;
  } else {
    // there is no intersected obj
    if (auto* obj = this->focused_obj_.lock()) {
      obj->leave();
      obj->frame();
    }
    this->focused_obj_.reset();
    this->set_surface_as_cursor(nullptr, 0, 0);
  }
}

bool Seat::set_focused_obj(util::WeakPtr<input::BoundedObject> obj) {
  if (obj == this->focused_obj_) {
    return false;
  }
  if (auto* obj = this->focused_obj_.lock()) {
    obj->leave();
    obj->frame();
  }
  this->focused_obj_.swap(obj);
  return true;
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
    auto      length = this->cursor_.lock() ? this->cursor_distance_ * 0.9F :
                                              Seat::kDefaultRayLen;
    glm::mat4 mat    = glm::translate(glm::mat4(1.F), this->kOrigin) *
                    glm::toMat4(this->ray_.rot) *
                    glm::scale(glm::mat4(1.F), glm::vec3(length));
    this->ray_renderer_->set_uniform_matrix(0, "local_model", mat);
  }
}
}  // namespace yaza::input
