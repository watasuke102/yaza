#pragma once

#include <wayland-server-core.h>
#include <wayland-server.h>
#include <wayland-util.h>

#include <cstdint>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/vector_float3.hpp>
#include <list>

#include "common.hpp"

namespace yaza::zwin::region {
struct CuboidRegion {
  glm::vec3 half_size;
  glm::vec3 center;
  glm::quat quat;
};

class Region {
 public:
  DISABLE_MOVE_AND_COPY(Region);
  explicit Region(wl_resource* resource);
  ~Region();

  void add_cuboid(glm::vec3 half_size, glm::vec3 center, glm::quat quat);

  std::list<CuboidRegion> regions;

 private:
  wl_resource* resource_;
};

void create(wl_client* client, uint32_t id);
}  // namespace yaza::zwin::region
