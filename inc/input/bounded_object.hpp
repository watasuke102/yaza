#pragma once

#include <wayland-server.h>

#include <glm/ext/vector_float3.hpp>
#include <optional>

#include "util/box.hpp"

namespace yaza::input {
// FIXME: are there any better data structures?
struct IntersectInfo {
  // in; for BoundedApp
  glm::vec3 origin;
  glm::vec3 direction;
  // out
  float     distance;
  glm::vec3 pos;  // for Surface
};

class BoundedObject {
 public:
  explicit BoundedObject(util::Box box) : geom_(box) {
  }
  BoundedObject()                                = default;
  BoundedObject(const BoundedObject&)            = default;
  BoundedObject(BoundedObject&&)                 = default;
  BoundedObject& operator=(const BoundedObject&) = default;
  BoundedObject& operator=(BoundedObject&&)      = default;
  virtual ~BoundedObject()                       = default;
  bool operator==(const BoundedObject& other) const {
    return this->resource() == other.resource();
  }
  bool operator!=(const BoundedObject& other) const {
    return !(*this == other);
  }

  virtual void enter(IntersectInfo& intersect_info)                   = 0;
  virtual void leave()                                                = 0;
  virtual void motion(IntersectInfo& intersect_info)                  = 0;
  virtual void button(uint32_t button, wl_pointer_button_state state) = 0;
  virtual void axis(float amount)                                     = 0;
  virtual void frame()                                                = 0;

  const util::Box& geometry() {
    return this->geom_;
  }
  virtual std::optional<IntersectInfo> check_intersection(
      const glm::vec3& origin, const glm::vec3& direction)              = 0;
  virtual void                       move(float polar, float azimuthal) = 0;
  [[nodiscard]] virtual bool         is_active()                        = 0;
  [[nodiscard]] virtual wl_resource* resource() const                   = 0;
  [[nodiscard]] virtual wl_client*   client() const                     = 0;

 protected:
  util::Box geom_;  // NOLINT
};
}  // namespace yaza::input
