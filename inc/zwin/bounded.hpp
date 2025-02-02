#pragma once

#include <wayland-server-core.h>
#include <wayland-util.h>

#include <cstdint>
#include <functional>
#include <list>
#include <unordered_map>

#include "common.hpp"
#include "input/bounded_object.hpp"
#include "util/weakable_unique_ptr.hpp"
#include "zwin/region.hpp"
#include "zwin/virtual_object.hpp"

namespace yaza::zwin::bounded {
// owned by VirtualObject, weakly referred by Seat
class BoundedApp : public input::BoundedObject {
 public:
  DISABLE_MOVE_AND_COPY(BoundedApp);
  BoundedApp(wl_resource*                  resource,
      zwin::virtual_object::VirtualObject* virtual_object);
  ~BoundedApp();

  void enter(input::IntersectInfo& intersect_info) override;
  void leave() override;
  void motion(input::IntersectInfo& intersect_info) override;
  void button(uint32_t button, wl_pointer_button_state state) override;
  void axis(float amount) override;
  void frame() override;

  std::optional<input::IntersectInfo> check_intersection(
      const glm::vec3& origin, const glm::vec3& direction) override;
  void                       move(float polar, float azimuthal) override;
  [[nodiscard]] bool         is_active() override;
  [[nodiscard]] wl_resource* resource() const override {
    return this->resource_;
  }
  [[nodiscard]] wl_client* client() const override {
    return wl_resource_get_client(this->resource_);
  }

  void set_region(util::UniPtr<region::Region>* region);
  void configure(wl_array* half_size);
  void ack_configure(uint32_t serial);
  void commit();

 private:
  struct {
    glm::vec3                       half_size = glm::vec3(0.F);
    std::list<region::CuboidRegion> regions;
  } pending_, current_;
  void foreach_ray(std::function<void(wl_resource*)>&& handler) const;

  zwin::virtual_object::VirtualObject* virtual_object_;
  util::Listener<std::nullptr_t*>      virtual_object_committed_listener_;

  std::unordered_map<uint32_t /* serial */, glm::vec3 /* half_size */>
               configure_list_;
  wl_resource* resource_;
};

void create(wl_client* client, uint32_t id, wl_array* half_size,
    zwin::virtual_object::VirtualObject* virtual_object);
}  // namespace yaza::zwin::bounded
