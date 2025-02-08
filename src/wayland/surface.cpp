#include "wayland/surface.hpp"

#include <GLES3/gl32.h>
#include <linux/input-event-codes.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>
#include <wayland-util.h>

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/geometric.hpp>
#include <memory>
#include <optional>

#include "common.hpp"
#include "input/bounded_object.hpp"
#include "remote/session.hpp"
#include "renderer.hpp"
#include "server.hpp"
#include "util/box.hpp"
#include "util/intersection.hpp"
#include "util/time.hpp"
#include "util/visitor_list.hpp"
#include "util/weakable_unique_ptr.hpp"
#include "xdg_shell/xdg_toplevel.hpp"

namespace yaza::wayland::surface {
namespace {
// clang-format off
constexpr auto* kVertShader = GLSL(
  uniform mat4 zVP;
  uniform mat4 local_model;
  layout(location = 0) in vec3 pos;
  layout(location = 1) in vec2 vertex_uv;

  out vec2 uv;

  void main() {
    gl_Position = zVP * local_model * vec4(pos, 1.0);
    uv          = vertex_uv;
  }
);
constexpr auto* kFragShader = GLSL(
  uniform sampler2D texture;
  in  vec2 uv;
  out vec4 color_out;

  void main() {
    vec4 color = texture(texture, uv);
    if (color.a < 0.5) discard;
    color_out = color;
  }
);
// clang-format on
constexpr float kPixelPerMeter    = 9000.F;
constexpr float kRadiusFromOrigin = 0.4F;
constexpr float kOffsetY          = 0.85F;
}  // namespace
Surface::Surface(wl_resource* resource)
    : input::BoundedObject(util::Box(
          glm::vec3(0.F, 0.85F, 0.F), glm::quat(), glm::vec3{1.F, 1.F, 0.F}))
    , resource_(resource) {
  if (server::get().remote->has_session()) {
    this->init_renderer();
  }
  this->session_established_listener_.set_handler(
      [this](remote::Session* /*session*/) {
        this->init_renderer();
      });
  server::get().remote->listen_session_established(
      this->session_established_listener_);

  this->session_disconnected_listener_.set_handler(
      [this](std::nullptr_t* /*data*/) {
        this->renderer_ = nullptr;
      });
  server::get().remote->listen_session_disconnected(
      this->session_disconnected_listener_);

  wl_list_init(&this->pending_.frame_callback_list);
  wl_list_init(&this->current_.frame_callback_list);
  this->session_frame_listener_.set_handler([this](std::nullptr_t* /*data*/) {
    auto         now_msec = util::now_msec();
    wl_resource* callback = nullptr;
    wl_resource* tmp      = nullptr;
    wl_resource_for_each_safe(
        callback, tmp, &this->current_.frame_callback_list) {
      wl_callback_send_done(callback, now_msec);
      wl_resource_destroy(callback);
    }
  });
  server::get().remote->listen_session_frame(this->session_frame_listener_);

  LOG_DEBUG("constructor: wl_surface@%u", wl_resource_get_id(this->resource_));
}
Surface::~Surface() {
  wl_list_remove(&this->pending_.frame_callback_list);
  wl_list_remove(&this->current_.frame_callback_list);
  LOG_DEBUG(" destructor: wl_surface@%u", wl_resource_get_id(this->resource_));
}

void Surface::enter(input::IntersectInfo& intersect_info) {
  server::get().seat->client_seats[this->client()]->pointer_foreach(
      [this, serial = server::get().next_serial(),
          x = wl_fixed_from_double(intersect_info.pos.x),
          y = wl_fixed_from_double(intersect_info.pos.y)](
          wl_resource* wl_pointer) {
        wl_pointer_send_enter(wl_pointer, serial, this->resource(), x, y);
      });
}
void Surface::leave() {
  server::get().seat->client_seats[this->client()]->pointer_foreach(
      [this, serial = server::get().next_serial()](wl_resource* wl_pointer) {
        wl_pointer_send_leave(wl_pointer, serial, this->resource());
      });
}
void Surface::motion(input::IntersectInfo& intersect_info) {
  server::get().seat->client_seats[this->client()]->pointer_foreach(
      [serial = server::get().next_serial(),
          x   = wl_fixed_from_double(intersect_info.pos.x),
          y   = wl_fixed_from_double(intersect_info.pos.y)](
          wl_resource* wl_pointer) {
        wl_pointer_send_motion(wl_pointer, serial, x, y);
      });
}
void Surface::button(uint32_t button, wl_pointer_button_state state) {
  server::get().seat->client_seats[this->client()]->pointer_foreach(
      [serial = server::get().next_serial(), now = util::now_msec(), button,
          state](wl_resource* wl_pointer) {
        wl_pointer_send_button(wl_pointer, serial, now, button, state);
      });
  if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
    auto surface = server::get().get_surface_from_resource(this->resource());
    if (surface.has_value()) {
      server::get().seat->set_keyboard_focused_surface(surface.value());
    }
  }
}
void Surface::axis(float amount) {
  server::get().seat->client_seats[this->client()]->pointer_foreach(
      [now = util::now_msec(), amount = wl_fixed_from_double(amount)](
          wl_resource* wl_pointer) {
        wl_pointer_send_axis(
            wl_pointer, now, WL_POINTER_AXIS_VERTICAL_SCROLL, amount);
      });
}
void Surface::frame() {
  server::get().seat->client_seats[this->client()]->pointer_foreach(
      [](wl_resource* wl_pointer) {
        wl_pointer_send_frame(wl_pointer);
      });
}
std::optional<input::IntersectInfo> Surface::check_intersection(
    const glm::vec3& origin, const glm::vec3& direction) {
  if (!this->texture_.has_data() || this->role_ == Role::CURSOR) {
    return std::nullopt;
  }
  auto      geom_mat          = this->geom_.mat();
  glm::vec3 vert_left_bottom  = geom_mat * glm::vec4(-1.F, -1.F, 0.F, 1.F);
  glm::vec3 vert_right_bottom = geom_mat * glm::vec4(+1.F, -1.F, 0.F, 1.F);
  glm::vec3 vert_left_top     = geom_mat * glm::vec4(-1.F, +1.F, 0.F, 1.F);
  auto      result            = util::intersection::with_surface(
      origin, direction, vert_left_bottom, vert_right_bottom, vert_left_top);
  if (!result.has_value()) {
    return std::nullopt;
  }
  return input::IntersectInfo{
      .origin    = origin,
      .direction = direction,
      .distance  = result->distance,
      .pos{static_cast<float>(this->tex_width_) * result->u,
           static_cast<float>(this->tex_height_) * (1.F - result->v), 0.F}
  };
}

void Surface::init_renderer() {
  this->renderer_ = std::make_unique<Renderer>(kVertShader, kFragShader);
  std::vector<float> vertices{
      +1.F, +1.F, 0.F,  // 3 ------ 0
      +1.F, -1.F, 0.F,  // |        |
      -1.F, -1.F, 0.F,  // |        |
      -1.F, +1.F, 0.F,  // 2 ------ 1
  };
  std::vector<float> uv{
      1.F, 0.F,  //
      1.F, 1.F,  //
      0.F, 1.F,  //
      0.F, 0.F,  //
  };
  this->renderer_->register_buffer(0, 3, GL_FLOAT, vertices.data(),
      sizeof(float) * vertices.size());  // NOLINT
  this->renderer_->register_buffer(
      1, 2, GL_FLOAT, uv.data(), sizeof(float) * uv.size());  // NOLINT
  this->renderer_->request_draw_arrays(GL_TRIANGLE_FAN, 0, 4);

  update_pos_and_rot();
  if (this->texture_.has_data()) {
    this->renderer_->set_texture(
        this->texture_, this->tex_width_, this->tex_height_);
    this->renderer_->commit();
  }
}

void Surface::update_pos_and_rot() {
  this->geom_.x() =
      kRadiusFromOrigin * sin(this->polar_) * sin(this->azimuthal_);
  this->geom_.y() = kOffsetY + kRadiusFromOrigin * cos(this->polar_);
  this->geom_.z() =
      kRadiusFromOrigin * sin(this->polar_) * cos(this->azimuthal_);

  // add $pi$ to $azimuthal$ to let Surface look at the camera
  this->geom_.rot() =
      glm::angleAxis(std::numbers::pi_v<float> + this->azimuthal_,
          glm::vec3{0.F, 1.F, 0.F}) *
      glm::angleAxis((std::numbers::pi_v<float> / 2.F) - this->polar_,
          glm::vec3{1.F, 0.F, 0.F});
  // updating geom_.size is the responsibility of Surface::set_texture_size()

  if (this->renderer_) {
    this->sync_geom();
  }
}
void Surface::sync_geom() {
  auto pos =
      this->geom_.pos() +
      (glm::vec3(this->offset_.x, -this->offset_.y, 0.F) / kPixelPerMeter);
  auto mat = glm::translate(glm::mat4(1.F), pos) * this->geom_.rotation_mat() *
             (this->is_active_ ? this->geom_.scale_mat() : glm::mat4(0.F));
  this->renderer_->set_uniform_matrix(0, "local_model", mat);
}
void Surface::set_texture_size(uint32_t width, uint32_t height) {
  this->tex_width_     = width;
  this->tex_height_    = height;
  this->geom_.width()  = static_cast<float>(this->tex_width_) / kPixelPerMeter;
  this->geom_.height() = static_cast<float>(this->tex_height_) / kPixelPerMeter;
  if (this->renderer_) {
    this->sync_geom();
  }
}

void Surface::set_role(Role role, RoleObject role_obj) {
  this->role_     = role;
  this->role_obj_ = role_obj;
}
void Surface::set_offset(glm::ivec2 offset) {
  this->pending_.offset_changed = true;
  this->pending_.offset         = offset;
}
void Surface::set_active(bool active) {
  this->is_active_ = active;
  if (this->renderer_) {
    this->sync_geom();
    this->renderer_->commit();
  }
}

void Surface::on_focus() {
  util::VisitorList([](xdg_shell::xdg_toplevel::XdgTopLevel*& xdg_toplevel) {
    xdg_toplevel->set_activated(true);
  }).visit(this->role_obj_);
}
void Surface::on_unfocus() {
  util::VisitorList([](xdg_shell::xdg_toplevel::XdgTopLevel*& xdg_toplevel) {
    xdg_toplevel->set_activated(false);
  }).visit(this->role_obj_);
}

void Surface::move(float polar, float azimuthal) {
  this->polar_ += polar;
  this->azimuthal_ += azimuthal;
  update_pos_and_rot();

  if (this->renderer_) {
    this->renderer_->commit();
  }
}
void Surface::move(glm::vec3 pos, glm::quat rot) {
  auto hotspot = std::get<glm::ivec2>(this->role_obj_);
  this->geom_.pos() =
      pos - (glm::vec3{hotspot.x, -hotspot.y, 0.F} / kPixelPerMeter);
  this->geom_.x() +=
      static_cast<float>(this->tex_width_) / 2.F / kPixelPerMeter;
  this->geom_.y() -=
      static_cast<float>(this->tex_height_) / 2.F / kPixelPerMeter;

  this->geom_.rot() =  // add rotation to let Surface look at the camera
      rot * glm::angleAxis(std::numbers::pi_v<float>, glm::vec3{0.F, 1.F, 0.F});

  if (this->renderer_) {
    this->sync_geom();
    this->renderer_->commit();
  }
}

void Surface::attach(wl_resource* buffer) {
  if (buffer == nullptr) {
    this->pending_.buffer = std::nullopt;
  } else {
    this->pending_.buffer = buffer;
  }
}
void Surface::queue_frame_callback(wl_resource* callback_resource) const {
  wl_list_insert(this->pending_.frame_callback_list.prev,
      wl_resource_get_link(callback_resource));
}

void Surface::listen_committed(util::Listener<std::nullptr_t*>& listener) {
  this->events_.committed.add_listener(listener);
}

void Surface::commit() {
  if (this->pending_.offset_changed) {
    this->offset_ += this->pending_.offset;
    this->pending_.offset_changed = false;
    if (this->renderer_) {
      this->sync_geom();
    }
  }

  if (this->pending_.buffer.has_value()) {
    if (this->texture_.has_data()) {
      this->texture_.reset();
    }
    auto*          buffer     = this->pending_.buffer.value();
    wl_shm_buffer* shm_buffer = wl_shm_buffer_get(buffer);
    auto           format     = wl_shm_buffer_get_format(shm_buffer);
    if (format == WL_SHM_FORMAT_ARGB8888 || format == WL_SHM_FORMAT_XRGB8888) {
      this->texture_.read_wl_surface_texture(shm_buffer);
      this->set_texture_size(wl_shm_buffer_get_width(shm_buffer),
          wl_shm_buffer_get_height(shm_buffer));
    } else {
      LOG_ERR("yaza does not support surface buffer format (%u)", format);
    }
    wl_buffer_send_release(buffer);
    this->pending_.buffer = std::nullopt;
  }

  if (this->renderer_ != nullptr && this->texture_.has_data()) {
    this->renderer_->set_texture(
        this->texture_, this->tex_width_, this->tex_height_);
    this->renderer_->commit();
  }

  wl_list_insert_list(
      &this->current_.frame_callback_list, &this->pending_.frame_callback_list);
  wl_list_init(&this->pending_.frame_callback_list);

  this->events_.committed.emit(nullptr);
}

namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void attach(wl_client* /*client*/, wl_resource* resource,
    wl_resource* buffer_resource, int32_t dx, int32_t dy) {
  auto* surface =
      static_cast<util::UniPtr<Surface>*>(wl_resource_get_user_data(resource));
  (*surface)->attach(buffer_resource);
  (*surface)->set_offset({dx, dy});
}
void damage(wl_client* /*client*/, wl_resource* /*resource*/, int32_t /*x*/,
    int32_t /*y*/, int32_t /*width*/, int32_t /*height*/) {
  // TODO
}
void frame(wl_client* client, wl_resource* resource, uint32_t callback) {
  wl_resource* callback_resource =
      wl_resource_create(client, &wl_callback_interface, 1, callback);
  if (!callback_resource) {
    wl_resource_post_no_memory(resource);
    return;
  }
  wl_resource_set_implementation(
      callback_resource, nullptr, nullptr, [](wl_resource* resource) {
        wl_list_remove(wl_resource_get_link(resource));
      });

  auto* self =
      static_cast<util::UniPtr<Surface>*>(wl_resource_get_user_data(resource));
  (*self)->queue_frame_callback(callback_resource);
}
void set_opaque_region(wl_client* /*client*/, wl_resource* /*resource*/,
    wl_resource* /*region_resource*/) {
  // TODO
}
void set_input_region(wl_client* /*client*/, wl_resource* /*resource*/,
    wl_resource* /*region_resource*/) {
  // TODO
}
void commit(wl_client* /*client*/, wl_resource* resource) {
  auto* self =
      static_cast<util::UniPtr<Surface>*>(wl_resource_get_user_data(resource));
  (*self)->commit();
}
void set_buffer_transform(
    wl_client* /*client*/, wl_resource* /*resource*/, int32_t /*transform*/) {
  // TODO
}
void set_buffer_scale(
    wl_client* /*client*/, wl_resource* /*resource*/, int32_t /*scale*/) {
  // TODO
}
void damage_buffer(wl_client* /*client*/, wl_resource* /*resource*/,
    int32_t /*x*/, int32_t /*y*/, int32_t /*width*/, int32_t /*height*/) {
  // TODO
}
void offset(
    wl_client* /*client*/, wl_resource* resource, int32_t dx, int32_t dy) {
  auto* surface =
      static_cast<util::UniPtr<Surface>*>(wl_resource_get_user_data(resource));
  (*surface)->set_offset({dx, dy});
}
constexpr struct wl_surface_interface kImpl = {
    .destroy              = destroy,
    .attach               = attach,
    .damage               = damage,
    .frame                = frame,
    .set_opaque_region    = set_opaque_region,
    .set_input_region     = set_input_region,
    .commit               = commit,
    .set_buffer_transform = set_buffer_transform,
    .set_buffer_scale     = set_buffer_scale,
    .damage_buffer        = damage_buffer,
    .offset               = offset,
};

void destroy(wl_resource* resource) {
  auto* surface =
      static_cast<util::UniPtr<Surface>*>(wl_resource_get_user_data(resource));
  delete surface;
}
}  // namespace

void create(wl_client* client, int version, uint32_t id) {
  auto* surface_resource =
      wl_resource_create(client, &wl_surface_interface, version, id);
  if (surface_resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  auto obj = std::dynamic_pointer_cast<input::BoundedObject>(
      std::make_shared<Surface>(surface_resource));
  auto* surface = new util::UniPtr<input::BoundedObject>(std::move(obj));
  wl_resource_set_implementation(surface_resource, &kImpl, surface, destroy);
  server::get().add_surface(surface->weak());
}
}  // namespace yaza::wayland::surface
