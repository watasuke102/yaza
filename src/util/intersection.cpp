#include "util/intersection.hpp"

#include <algorithm>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/vector_query.hpp>
#include <limits>
#include <optional>

namespace yaza::util::intersection {
// using Möller–Trumbore intersection algorithm
std::optional<SurfaceInfo> with_surface(const glm::vec3& o,
    const glm::vec3& dir, const glm::vec3& v0, const glm::vec3& v1,
    const glm::vec3& v2) {
  const glm::vec3 d   = glm::normalize(dir);
  // Vectors that express the 'E'dges of the plane
  const glm::vec3 e1  = v1 - v0;
  const glm::vec3 e2  = v2 - v0;
  const glm::vec3 w   = o - v0;  // `T` in the original thesis
  // The any point P on the plane can be exprssed as:
  //   $P = vert0 + u*e0 + v*e1  (0 <= u, v <= 1)$
  // Note tht original algorithm limits u+v <= 1 because its target is triangle
  // If P is also expressed as follows, the vector and the plane is intersected
  //   $origin + t*direction (t >= 0)$
  const glm::vec3 p   = glm::cross(d, e2);
  const glm::vec3 q   = glm::cross(w, e1);
  const float     det = glm::dot(p, e1);
  const float     t   = glm::dot(q, e2) / det;
  if (t < 0) {
    return std::nullopt;
  }
  const float u = glm::dot(p, w) / det;
  if (u < 0 || u > 1) {
    return std::nullopt;
  }
  const float v = glm::dot(q, d) / det;
  if (v < 0 || v > 1) {
    return std::nullopt;
  }
  return SurfaceInfo{.distance = t, .u = u, .v = v};
}

// based
// https://www.opengl-tutorial.org/jp/miscellaneous/clicking-on-objects/picking-with-custom-ray-obb-function/
std::optional<float> with_obb(const glm::vec3& origin,
    const glm::vec3& direction, const glm::vec3& half_size,
    const glm::mat4& model_mat) {
  const glm::vec3 direction_norm = glm::normalize(direction);
  const glm::vec3 obb_world_pos  = model_mat[3];
  const glm::vec3 delta          = obb_world_pos - origin;

  float near = 0.F;
  float far  = std::numeric_limits<float>::max();
  for (int i = 0; i <= 2; ++i) {
    const glm::vec3 axis = model_mat[i];
    const float     e    = glm::dot(axis, delta);
    const float     f    = glm::dot(direction_norm, axis);
    if (std::abs(f) > 0.001F) {
      float intersect_min = (e - half_size[i]) / f;
      float intersect_max = (e + half_size[i]) / f;
      if (intersect_min > intersect_max) {
        std::swap(intersect_min, intersect_max);
      }
      far  = std::min(far, intersect_max);
      near = std::max(near, intersect_min);
      if (far < near) {
        return std::nullopt;
      }
    } else if (-e - half_size[i] > 0.F || -e + half_size[i] < 0.F) {
      return std::nullopt;
    }
  }
  return near;
}
}  // namespace yaza::util::intersection
