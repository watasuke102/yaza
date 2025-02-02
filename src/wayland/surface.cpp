#include "wayland/surface.hpp"

#include <GLES3/gl32.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>
#include <wayland-util.h>

#include <cassert>
#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/ext/vector_float3.hpp>
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
#include "util/weakable_unique_ptr.hpp"

namespace yaza::wayland::surface {
namespace {
// clang-format off
constexpr auto* kVertShader = GLSL(
  uniform mat4 zMVP;
  uniform mat4 surface_scale;
  layout(location = 0) in vec3 pos;
  layout(location = 1) in vec2 vertex_uv;

  out vec2 uv;

  void main() {
    gl_Position = zMVP * surface_scale * vec4(pos, 1.0);
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

  LOG_DEBUG("constructor: wl_surface@%u", wl_resource_get_id(this->resource_));
}
Surface::~Surface() {
  LOG_DEBUG(" destructor: wl_surface@%u", wl_resource_get_id(this->resource_));
}

void Surface::enter(input::IntersectInfo& intersect_info) {
  if (auto* wl_pointer =
          server::get().seat->pointer_resources[this->client()]) {
    wl_pointer_send_enter(wl_pointer, server::get().next_serial(),
        this->resource(), wl_fixed_from_double(intersect_info.pos.x),
        wl_fixed_from_double(intersect_info.pos.y));
  }
}
void Surface::leave() {
  if (auto* wl_pointer =
          server::get().seat->pointer_resources[this->client()]) {
    wl_pointer_send_leave(
        wl_pointer, server::get().next_serial(), this->resource());
  }
}
void Surface::motion(input::IntersectInfo& intersect_info) {
  if (auto* wl_pointer =
          server::get().seat->pointer_resources[this->client()]) {
    wl_pointer_send_motion(wl_pointer, util::now_msec(),
        wl_fixed_from_double(intersect_info.pos.x),
        wl_fixed_from_double(intersect_info.pos.y));
  }
}
void Surface::button(uint32_t button, wl_pointer_button_state state) {
  if (auto* wl_pointer =
          server::get().seat->pointer_resources[this->client()]) {
    wl_pointer_send_button(wl_pointer, server::get().next_serial(),
        util::now_msec(), button, state);
  }
}
void Surface::axis(float amount) {
  if (auto* wl_pointer =
          server::get().seat->pointer_resources[this->client()]) {
    wl_pointer_send_axis(wl_pointer, util::now_msec(),
        WL_POINTER_AXIS_VERTICAL_SCROLL, wl_fixed_from_double(amount));
  }
}
void Surface::frame() {
  if (auto* wl_pointer =
          server::get().seat->pointer_resources[this->client()]) {
    wl_pointer_send_frame(wl_pointer);
  }
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
      .distance = result->distance,
      .pos{static_cast<float>(this->tex_width_) * result->u,
           static_cast<float>(this->tex_height_) * (1.F - result->v), 0.F}
  };
}
bool Surface::is_active() {
  return server::get().seat->pointer_resources[this->client()] != nullptr;
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
  this->geom_.y() = 0.85F + kRadiusFromOrigin * cos(this->polar_);
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
  auto scale = this->is_active_ ? this->geom_.scale_mat() : glm::mat4(0.F);
  this->renderer_->set_uniform_matrix(0, "surface_scale", scale);
  this->renderer_->move_abs(this->geom_.pos());
  this->renderer_->set_rot(this->geom_.rot());
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

void Surface::set_role(Role role) {
  this->role_ = role;
}
void Surface::set_offset(glm::ivec2 offset) {
  this->pending_.offset = offset;
}
void Surface::set_active(bool active) {
  this->is_active_ = active;
  if (this->renderer_) {
    this->sync_geom();
    this->renderer_->commit();
  }
}
void Surface::move(float polar, float azimuthal) {
  this->polar_ += polar;
  this->azimuthal_ += azimuthal;
  update_pos_and_rot();

  if (this->renderer_) {
    this->renderer_->commit();
  }
}
void Surface::move(glm::vec3 pos, glm::quat rot, glm::ivec2 hotspot) {
  // FIXME: offset isn't applied until the ray moves
  glm::vec3 top_left(this->offset_ - hotspot, 0.F);
  top_left.y *= -1;
  this->geom_.pos() = pos - (top_left / kPixelPerMeter);
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
void Surface::set_callback(wl_resource* resource) {
  this->pending_.callback = resource;
}

void Surface::listen_committed(util::Listener<std::nullptr_t*>& listener) {
  this->events_.committed.add_listener(listener);
}

void Surface::commit() {
  this->offset_ -= this->pending_.offset;

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

  if (this->pending_.callback.has_value()) {
    auto* callback = this->pending_.callback.value();
    wl_callback_send_done(callback, util::now_msec());
    wl_resource_destroy(callback);
    this->pending_.callback = std::nullopt;
  }

  if (this->renderer_ != nullptr && this->texture_.has_data()) {
    this->renderer_->set_texture(
        this->texture_, this->tex_width_, this->tex_height_);
    this->renderer_->commit();
  }

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
  if (callback_resource) {
    auto* self = static_cast<util::UniPtr<Surface>*>(
        wl_resource_get_user_data(resource));
    (*self)->set_callback(callback_resource);
  } else {
    wl_client_post_no_memory(client);
  }
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
  server::get().surfaces.emplace_back(surface->weak());
}
}  // namespace yaza::wayland::surface
