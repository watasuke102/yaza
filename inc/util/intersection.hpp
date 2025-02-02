#pragma once

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <optional>

namespace yaza::util::intersection {
struct SurfaceInfo {
  float distance;
  float u, v;
};
/// return intersection info if the vector is intersected with plane
/// @param origin    The starting point of the vector
/// @param direction The direction of the vector
std::optional<SurfaceInfo> with_surface(const glm::vec3& origin,
    const glm::vec3& direction, const glm::vec3& vert_left_bottom,
    const glm::vec3& vert_right_bottom, const glm::vec3& vert_left_top);

/// return distance if the vector is intersected with oriented bounding box
/// @param origin    The starting point of the vector
/// @param direction The direction of the vector
/// @param half_size The size of axis-aligned bounding box
std::optional<float> with_obb(const glm::vec3& origin,
    const glm::vec3& direction, const glm::vec3& half_size,
    const glm::mat4& model_mat);
}  // namespace yaza::util::intersection
