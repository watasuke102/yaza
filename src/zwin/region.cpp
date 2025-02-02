#include "zwin/region.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>
#include <wayland-util.h>
#include <zen-remote/server/virtual-object.h>
#include <zwin-protocol.h>

#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include "common.hpp"
#include "util/convert.hpp"
#include "util/weakable_unique_ptr.hpp"

namespace yaza::zwin::region {
Region::Region(wl_resource* resource) : resource_(resource) {
  LOG_DEBUG("constructor: zwin Region#%d", wl_resource_get_id(this->resource_));
}
Region::~Region() {
  LOG_DEBUG(" destructor: zwin Region#%d", wl_resource_get_id(this->resource_));
}

void Region::add_cuboid(glm::vec3 half_size, glm::vec3 center, glm::quat quat) {
  regions.emplace_back(half_size, center, quat);
}

namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void add_cuboid(struct wl_client* /*client*/, struct wl_resource* resource,
    struct wl_array* half_size_array, struct wl_array* center_array,
    struct wl_array* quat_array) {
  auto* self =
      static_cast<util::UniPtr<Region>*>(wl_resource_get_user_data(resource));
  glm::vec3 half_size;
  glm::vec3 center;
  glm::quat quat;
  if (!util::convert::from_wl_array(half_size_array, &half_size)) {
    wl_resource_post_error(resource, ZWN_COMPOSITOR_ERROR_WL_ARRAY_SIZE,
        "half_size array size (%ld) does not equal expeted size (%ld)",
        half_size_array->size, sizeof(half_size));
    return;
  }
  if (!util::convert::from_wl_array(center_array, &center)) {
    wl_resource_post_error(resource, ZWN_COMPOSITOR_ERROR_WL_ARRAY_SIZE,
        "center array size (%ld) does not equal expeted size (%ld)",
        center_array->size, sizeof(center));
    return;
  }
  if (!util::convert::from_wl_array(quat_array, &quat)) {
    wl_resource_post_error(resource, ZWN_COMPOSITOR_ERROR_WL_ARRAY_SIZE,
        "quat array size (%ld) does not equal expeted size (%ld)",
        quat_array->size, sizeof(quat));
    return;
  }
  (*self)->add_cuboid(half_size, center, quat);
}
void add_sphere(struct wl_client* /*client*/, struct wl_resource* /*resource*/,
    struct wl_array* /*center*/, struct wl_array* /*radius*/) {
}
constexpr struct zwn_region_interface kImpl = {
    .destroy    = destroy,
    .add_cuboid = add_cuboid,
    .add_sphere = add_sphere,
};

void destroy(wl_resource* resource) {
  auto* self =
      static_cast<util::UniPtr<Region>*>(wl_resource_get_user_data(resource));
  delete self;
}
}  // namespace

void create(wl_client* client, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &zwn_region_interface, 1, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  auto* self = new util::UniPtr<Region>(resource);
  wl_resource_set_implementation(resource, &kImpl, self, destroy);
}
}  // namespace yaza::zwin::region
