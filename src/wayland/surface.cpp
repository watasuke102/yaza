#include "wayland/surface.hpp"

#include <GLES3/gl32.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <memory>
#include <optional>

#include "common.hpp"
#include "remote/remote.hpp"
#include "remote/session.hpp"
#include "renderer.hpp"
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
  out vec4 color;

  void main() {
    color = texture(texture, uv);
  }
);
// clang-format on
constexpr float kPixelPerMeter    = 9000.F;
constexpr float kRadiusFromOrigin = 0.4F;
}  // namespace
Surface::Surface(uint32_t id)
    : geom_(glm::vec3(0.F, 0.85F, -kRadiusFromOrigin),
          glm::quat(glm::vec3(0.F, 0.F, 0.F)), glm::vec3{1.F, 1.F, 0.F})
    , id_(id) {
  if (remote::g_remote->has_session()) {
    this->init_renderer();
  }
  this->session_established_listener_.set_handler(
      [this](remote::Session* /*session*/) {
        this->init_renderer();
      });
  remote::g_remote->listen_session_established(
      this->session_established_listener_);

  this->session_disconnected_listener_.set_handler(
      [this](std::nullptr_t* /*data*/) {
        this->renderer_ = nullptr;
      });
  remote::g_remote->listen_session_disconnected(
      this->session_disconnected_listener_);

  LOG_DEBUG("constructor: wl_surface@%u", this->id_);
}
Surface::~Surface() {
  LOG_DEBUG(" destructor: wl_surface@%u", this->id_);
}
void Surface::init_renderer() {
  this->renderer_       = std::make_unique<Renderer>(kVertShader, kFragShader);
  constexpr float kSize = 1.F;
  this->renderer_->move_abs(this->geom_.pos());
  this->renderer_->set_rot(this->geom_.rot());
  this->renderer_->set_vertex(std::vector<BufferElement>{
      /*
        3 -- 0
        |    |
        2 -- 1
        */
      {.x_ = +kSize, .y_ = +kSize, .z_ = -0.F, .u_ = 1.F, .v_ = 0.F},
      {.x_ = +kSize, .y_ = -kSize, .z_ = -0.F, .u_ = 1.F, .v_ = 1.F},
      {.x_ = -kSize, .y_ = -kSize, .z_ = -0.F, .u_ = 0.F, .v_ = 1.F},
      {.x_ = -kSize, .y_ = +kSize, .z_ = -0.F, .u_ = 0.F, .v_ = 0.F},
  });
  this->renderer_->request_draw_arrays(GL_TRIANGLE_FAN, 0, 4);
  this->update_geom();
  if (this->texture_.has_data()) {
    this->renderer_->set_texture(
        this->texture_, this->tex_width_, this->tex_height_);
    this->renderer_->commit();
  }
}
void Surface::update_geom() {
  this->geom_.width()  = static_cast<float>(this->tex_width_) / kPixelPerMeter;
  this->geom_.height() = static_cast<float>(this->tex_height_) / kPixelPerMeter;
  LOG_DEBUG("geom: %d, %d -> %.7f, %.7f", tex_width_, tex_height_,
      geom_.width(), geom_.height());
  if (!this->renderer_) {
    return;
  }
  glm::mat4 model_mat = glm::scale(glm::mat4(1.F), this->geom_.size());
  this->renderer_->set_uniform_matrix(0, "surface_scale", model_mat);
}

void Surface::attach(wl_resource* buffer, int32_t /*sx*/, int32_t /*sy*/) {
  // x, y is discarded but `Setting anything other than 0 as x and y arguments
  // is discouraged`, so ignore them for now (TODO?)
  if (buffer == nullptr) {
    this->pending_.buffer_ = std::nullopt;
  } else {
    this->pending_.buffer_ = buffer;
  }
}
void Surface::set_callback(wl_resource* resource) {
  this->pending_.callback_ = resource;
}

void Surface::listen_committed(util::Listener<std::nullptr_t*>& listener) {
  this->events_.committed_.add_listener(listener);
}

void Surface::commit() {
  if (this->pending_.buffer_.has_value()) {
    if (this->texture_.has_data()) {
      this->texture_.reset();
    }
    auto*          buffer     = this->pending_.buffer_.value();
    wl_shm_buffer* shm_buffer = wl_shm_buffer_get(buffer);
    auto           format     = wl_shm_buffer_get_format(shm_buffer);
    if (format == WL_SHM_FORMAT_ARGB8888 || format == WL_SHM_FORMAT_XRGB8888) {
      this->texture_.read_wl_surface_texture(shm_buffer);
      this->tex_width_  = wl_shm_buffer_get_width(shm_buffer);
      this->tex_height_ = wl_shm_buffer_get_height(shm_buffer);
      this->update_geom();
    } else {
      LOG_ERR("yaza does not support surface buffer format (%u)", format);
    }
    wl_buffer_send_release(buffer);
    this->pending_.buffer_ = std::nullopt;
  }

  if (this->pending_.callback_.has_value()) {
    auto* callback = this->pending_.callback_.value();
    wl_callback_send_done(callback, util::now_msec());
    wl_resource_destroy(callback);
    this->pending_.callback_ = std::nullopt;
  }

  if (this->renderer_ != nullptr && this->texture_.has_data()) {
    this->renderer_->set_texture(
        this->texture_, this->tex_width_, this->tex_height_);
    this->renderer_->commit();
  }

  this->events_.committed_.emit(nullptr);
}

namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void attach(wl_client* /*client*/, wl_resource* resource,
    wl_resource* buffer_resource, int32_t x, int32_t y) {
  auto* surface =
      static_cast<util::UniPtr<Surface>*>(wl_resource_get_user_data(resource));
  (*surface)->attach(buffer_resource, x, y);
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
void offset(wl_client* /*client*/, wl_resource* /*resource*/, int32_t /*x*/,
    int32_t /*y*/) {
  // TODO
}
const struct wl_surface_interface kImpl = {
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
  auto* surface = new util::UniPtr<Surface>(id);
  wl_resource_set_implementation(surface_resource, &kImpl, surface, destroy);
}
}  // namespace yaza::wayland::surface
