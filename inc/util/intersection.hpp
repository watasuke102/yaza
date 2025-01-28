#pragma once

#include <glm/ext/vector_float3.hpp>
#include <optional>

namespace yaza::util {
struct IntersectInfo {
  float distance;
  float u, v;
};
/// return intersection info if the vector is intersected with plane
/// @param origin    The starting point of the vector
/// @param direction The direction of the vector
std::optional<IntersectInfo> intersected_at(const glm::vec3& origin,
    const glm::vec3& direction, const glm::vec3& vert_left_bottom,
    const glm::vec3& vert_right_bottom, const glm::vec3& vert_left_top);
}  // namespace yaza::util
