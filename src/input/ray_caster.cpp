#include "input/ray_caster.hpp"

#include <GLES3/gl32.h>

#include "common.hpp"
#include "server.hpp"

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

RayCaster::RayCaster() {
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
}

glm::vec3 RayCaster::direction() const {
  return glm::rotate(this->rot_, RayCaster::kBaseDirection);
}
glm::quat RayCaster::rot() const {
  return this->rot_;
}

float RayCaster::move_rel(float polar, float azimuthal) {
  this->azimuthal_ += azimuthal;

  constexpr float kPolarMin  = std::numbers::pi / 8.F;
  constexpr float kPolarMax  = std::numbers::pi - kPolarMin;
  auto            diff_polar = this->polar_;
  this->polar_ = std::clamp(this->polar_ + polar, kPolarMin, kPolarMax);
  diff_polar   = this->polar_ - diff_polar;

  this->update_ray_rot();
  if (this->ray_renderer_) {
    this->ray_renderer_->commit();
  }

  return diff_polar;
}
void RayCaster::set_length(std::optional<float> len) {
  this->length_ = len.has_value() ? len.value() : RayCaster::kDefaultRayLen;
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
void RayCaster::init_ray_renderer() {
  this->ray_renderer_ = std::make_unique<Renderer>(kVertShader, kFragShader);
  std::vector<float> vertices = {
      0.F,
      0.F,
      0.F,  //
      RayCaster::kBaseDirection.x,
      RayCaster::kBaseDirection.y,
      RayCaster::kBaseDirection.z,
  };
  this->ray_renderer_->register_buffer(0, 3, GL_FLOAT, vertices.data(),
      sizeof(float) * vertices.size());  // NOLINT
  this->ray_renderer_->request_draw_arrays(GL_LINE_STRIP, 0, 2);
  this->update_ray_rot();
  this->ray_renderer_->commit();
}
void RayCaster::update_ray_rot() {
  this->rot_ = glm::angleAxis(this->azimuthal_, glm::vec3{0.F, 1.F, 0.F}) *
               glm::angleAxis((std::numbers::pi_v<float> / 2.F) - this->polar_,
                   glm::vec3{-1.F, 0.F, 0.F});
  if (this->ray_renderer_) {
    glm::mat4 mat = glm::translate(glm::mat4(1.F), RayCaster::kOrigin) *
                    glm::toMat4(this->rot_) *
                    glm::scale(glm::mat4(1.F), glm::vec3(this->length_));
    this->ray_renderer_->set_uniform_matrix(0, "local_model", mat);
  }
}

}  // namespace yaza::input
