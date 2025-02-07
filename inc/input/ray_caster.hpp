#pragma once

#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/vector_float3.hpp>
#include <memory>
#include <numbers>

#include "common.hpp"
#include "remote/session.hpp"
#include "renderer.hpp"
#include "util/signal.hpp"

namespace yaza::input {
class RayCaster {
 public:
  DISABLE_MOVE_AND_COPY(RayCaster);
  RayCaster();
  ~RayCaster() = default;

  constexpr static glm::vec3 kOrigin = glm::vec3(0.F, 0.849F, -0.001F);
  [[nodiscard]] glm::vec3    direction() const;
  [[nodiscard]] glm::quat    rot() const;

  /// return actually changed polar amount
  float move_rel(float polar, float azimuthal);
  /// length will be reset if nullopt is passed
  void  set_length(std::optional<float> len);

 private:
  constexpr static glm::vec3 kBaseDirection = glm::vec3(0.F, 0.F, 1.F);
  constexpr static float     kDefaultRayLen = 3.4F;

  float     length_    = kDefaultRayLen;
  float     polar_     = std::numbers::pi / 2.F;  // [0, pi]
  float     azimuthal_ = std::numbers::pi;        // x-z plane, [0, 2pi]
  glm::quat rot_;

  std::unique_ptr<Renderer> ray_renderer_;
  void                      init_ray_renderer();
  void                      update_ray_rot();

  util::Listener<remote::Session*> session_established_listener_;
  util::Listener<std::nullptr_t*>  session_disconnected_listener_;
};
}  // namespace yaza::input
