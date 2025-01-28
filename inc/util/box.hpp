#pragma once

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtx/quaternion.hpp>

namespace yaza::util {
class Box {
 public:
  Box() : pos_(), rot_(), size_() {
  }
  Box(glm::vec3 pos, glm::quat rot, glm::vec3 size)
      : pos_(pos), rot_(rot), size_(size) {
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
  glm::vec3& size() {
    return this->size_;
  }
  float& width() {
    return this->size_.x;
  }
  float& height() {
    return this->size_.y;
  }
  float& depth() {
    return this->size_.z;
  }

  glm::mat4 mat() {
    return glm::translate(glm::mat4(1.F), this->pos_) * glm::toMat4(this->rot_);
  }

 private:
  glm::vec3 pos_;
  glm::quat rot_;
  glm::vec3 size_;
};
}  // namespace yaza::util
