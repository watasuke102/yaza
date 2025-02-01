#pragma once

#include <wayland-server.h>

#include <glm/ext/vector_float3.hpp>
#include <optional>

#include "util/box.hpp"

namespace yaza::input {
struct IntersectInfo {
  float     distance;
  glm::vec3 pos;
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
  virtual bool operator==(const BoundedObject&)  = 0;
  bool         operator!=(const BoundedObject& other) {
    return !(*this == other);
  }

  virtual void enter(IntersectInfo& intersect_info)                   = 0;
  virtual void leave()                                                = 0;
  virtual void motion(IntersectInfo& intersect_info)                  = 0;
  virtual void button(uint32_t button, wl_pointer_button_state state) = 0;
  virtual void axis(float amount)                                     = 0;
  virtual void frame()                                                = 0;

  virtual std::optional<IntersectInfo> intersected_at(
      const glm::vec3& origin, const glm::vec3& direction)              = 0;
  virtual void                       move(float polar, float azimuthal) = 0;
  [[nodiscard]] virtual bool         is_active()                        = 0;
  [[nodiscard]] virtual wl_resource* resource() const                   = 0;
  [[nodiscard]] virtual wl_client*   client() const                     = 0;

 protected:
  util::Box geom_;
};
}  // namespace yaza::input
