#pragma once

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtx/quaternion.hpp>

namespace yaza::util {
class Box {
 public:
  Box() : pos_(), rot_(), scale_() {
  }
  Box(glm::vec3 pos, glm::quat rot, glm::vec3 scale)
      : pos_(pos), rot_(rot), scale_(scale) {
  }

  glm::vec3& pos() {
    return this->pos_;
  }
  float& x() {
    return this->pos_.x;
  }
  float& y() {
    return this->pos_.y;
  }
  float& z() {
    return this->pos_.z;
  }
  glm::quat& rot() {
    return this->rot_;
  }
  float& rx() {
    return this->rot_.x;
  }
  float& ry() {
    return this->rot_.y;
  }
  float& rz() {
    return this->rot_.z;
  }
  glm::vec3& scale() {
    return this->scale_;
  }
  float& width() {
    return this->scale_.x;
  }
  float& height() {
    return this->scale_.y;
  }
  float& depth() {
    return this->scale_.z;
  }

  glm::mat4 mat() {
    return this->translation_mat() * this->rotation_mat() * this->scale_mat();
  }
  glm::mat4 translation_mat() {
    return glm::translate(glm::mat4(1.F), this->pos_);
  }
  glm::mat4 rotation_mat() {
    return glm::toMat4(this->rot_);
  }
  glm::mat4 scale_mat() {
    return glm::scale(glm::mat4(1.F), this->scale_);
  }

 private:
  glm::vec3 pos_;
  glm::quat rot_;
  glm::vec3 scale_;
};
}  // namespace yaza::util
