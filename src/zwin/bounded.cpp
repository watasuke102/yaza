#include "zwin/bounded.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>
#include <wayland-util.h>
#include <zwin-protocol.h>
#include <zwin-shell-protocol.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <glm/gtc/type_ptr.hpp>
#include <optional>
#include <stdexcept>
#include <utility>

#include "common.hpp"
#include "input/bounded_object.hpp"
#include "server.hpp"
#include "util/convert.hpp"
#include "util/intersection.hpp"
#include "util/time.hpp"
#include "util/weakable_unique_ptr.hpp"

namespace yaza::zwin::bounded {
namespace {
constexpr float kRadiusFromOrigin = 2.2F;
constexpr float kOffsetY          = 1.F;
}  // namespace
BoundedApp::BoundedApp(
    wl_resource* resource, zwin::virtual_object::VirtualObject* virtual_object)
    : input::BoundedObject(
          util::Box(glm::vec3(), glm::quat(), glm::vec3{0.F, 0.F, 0.F}))
    , virtual_object_(virtual_object)
    , resource_(resource) {
  this->update_pos_and_rot();
  this->virtual_object_committed_listener_.set_handler(
      [this](std::nullptr_t* /**/) {
        this->commit();
      });
  this->virtual_object_->listen_commited(
      this->virtual_object_committed_listener_);
  LOG_DEBUG("constructor: BoundedApp#%d", wl_resource_get_id(this->resource_));
}
BoundedApp::~BoundedApp() {
  LOG_DEBUG(" destructor: BoundedApp#%d", wl_resource_get_id(this->resource_));
  wl_resource_set_user_data(this->resource_, nullptr);
  wl_resource_set_destructor(this->resource_, nullptr);
}

void BoundedApp::enter(input::IntersectInfo& intersect_info) {
  wl_array origin;
  wl_array_init(&origin);
  if (!util::convert::to_wl_array(&intersect_info.origin, &origin)) {
    wl_resource_post_no_memory(this->resource());
    return;
  }
  wl_array direction;
  wl_array_init(&direction);
  if (!util::convert::to_wl_array(&intersect_info.direction, &direction)) {
    wl_resource_post_no_memory(this->resource());
    return;
  }
  this->foreach_ray([this, &origin, &direction](wl_resource* zwn_ray) {
    zwn_ray_send_enter(zwn_ray, server::get().next_serial(),
        this->virtual_object_->resource(), &origin, &direction);
  });
  wl_array_release(&origin);
  wl_array_release(&direction);
}
void BoundedApp::leave() {
  this->foreach_ray([this](wl_resource* zwn_ray) {
    zwn_ray_send_leave(zwn_ray, server::get().next_serial(),
        this->virtual_object_->resource());
  });
}
void BoundedApp::motion(input::IntersectInfo& intersect_info) {
  wl_array origin;
  wl_array_init(&origin);
  if (!util::convert::to_wl_array(&intersect_info.origin, &origin)) {
    wl_resource_post_no_memory(this->resource());
    return;
  }
  wl_array direction;
  wl_array_init(&direction);
  if (!util::convert::to_wl_array(&intersect_info.direction, &direction)) {
    wl_resource_post_no_memory(this->resource());
    return;
  }
  this->foreach_ray([&origin, &direction](wl_resource* zwn_ray) {
    zwn_ray_send_motion(zwn_ray, util::now_msec(), &origin, &direction);
  });
  wl_array_release(&origin);
  wl_array_release(&direction);
}
void BoundedApp::button(uint32_t button, wl_pointer_button_state wl_state) {
  enum zwn_ray_button_state state = ZWN_RAY_BUTTON_STATE_PRESSED;
  switch (wl_state) {
    case WL_POINTER_BUTTON_STATE_PRESSED:
      state = ZWN_RAY_BUTTON_STATE_PRESSED;
      break;
    case WL_POINTER_BUTTON_STATE_RELEASED:
      state = ZWN_RAY_BUTTON_STATE_RELEASED;
      break;
    default:
      return;
  }
  this->foreach_ray([button, state](wl_resource* zwn_ray) {
    zwn_ray_send_button(
        zwn_ray, server::get().next_serial(), util::now_msec(), button, state);
  });
}
void BoundedApp::axis(float amount) {
  this->foreach_ray(
      [amount = wl_fixed_from_double(amount)](wl_resource* zwn_ray) {
        zwn_ray_send_axis(
            zwn_ray, util::now_msec(), ZWN_RAY_AXIS_VERTICAL_SCROLL, amount);
      });
}
void BoundedApp::frame() {
  this->foreach_ray([](wl_resource* zwn_ray) {
    zwn_ray_send_frame(zwn_ray);
  });
}

std::optional<input::IntersectInfo> BoundedApp::check_intersection(
    const glm::vec3& origin, const glm::vec3& direction) {
  const auto outer_mat =
      this->geom_.translation_mat() * this->geom_.rotation_mat();
  auto outer_distance = util::intersection::with_obb(
      origin, direction, this->current_.half_size, outer_mat);
  if (!outer_distance.has_value()) {
    return std::nullopt;
  }

  std::optional<float> inner_min_distance;
  for (auto& region : this->current_.regions) {
    const auto inner_mat = outer_mat *
                           glm::translate(glm::mat4(1.F), region.center) *
                           glm::toMat4(region.quat);
    auto result = util::intersection::with_obb(
        origin, direction, region.half_size, inner_mat);
    if (!result.has_value()) {
      continue;
    }
    if (!inner_min_distance.has_value() ||
        result.value() < inner_min_distance.value()) {
      inner_min_distance = result;
    }
  }
  if (!inner_min_distance.has_value()) {
    return std::nullopt;
  }

  return input::IntersectInfo{
      .origin    = origin,
      .direction = direction,
      .distance  = std::max(inner_min_distance.value(), outer_distance.value()),
      .pos       = glm::vec3(),
  };
}

bool BoundedApp::is_active() {
  return server::get().seat->ray_resources.count(this->client()) > 0;
}

void BoundedApp::set_region(util::UniPtr<region::Region>* region) {
  this->pending_.regions = (*region)->regions;
}
void BoundedApp::foreach_ray(
    std::function<void(wl_resource*)>&& handler) const {
  auto& rays  = server::get().seat->ray_resources;
  auto  index = rays.bucket(this->client());
  std::for_each(rays.begin(index), rays.end(index),
      [handler = std::move(handler)](std::pair<wl_client*, wl_resource*> e) {
        handler(e.second);
      });
}
void BoundedApp::configure(wl_array* half_size) {
  glm::vec3 requested_half_size;
  if (!util::convert::from_wl_array(half_size, &requested_half_size)) {
    wl_resource_post_error(this->resource(), ZWN_COMPOSITOR_ERROR_WL_ARRAY_SIZE,
        "half_size array size (%ld) does not equal expeted size (%ld)",
        half_size->size, sizeof(glm::vec3));
    return;
  }
  auto serial = server::get().next_serial();
  this->configure_list_.emplace(serial, requested_half_size);
  zwn_bounded_send_configure(this->resource(), half_size, serial);
}
void BoundedApp::ack_configure(uint32_t serial) {
  try {
    this->pending_.half_size = this->configure_list_.at(serial);
  } catch (std::out_of_range&) {
    LOG_WARN("Invalid ack_configure (serial: %u)", serial);
  }
}
void BoundedApp::commit() {
  this->current_.half_size = this->pending_.half_size;
  this->geom_.scale()      = this->pending_.half_size * 2.F;
  this->current_.regions   = this->pending_.regions;
}

void BoundedApp::update_pos_and_rot() {
  this->geom_.x() =
      kRadiusFromOrigin * sin(this->polar_) * sin(this->azimuthal_);
  this->geom_.y() = kOffsetY + kRadiusFromOrigin * cos(this->polar_);
  this->geom_.z() =
      kRadiusFromOrigin * sin(this->polar_) * cos(this->azimuthal_);

  this->geom_.rot() =
      glm::angleAxis(this->azimuthal_, glm::vec3{0.F, 1.F, 0.F}) *
      glm::angleAxis(this->polar_ - (std::numbers::pi_v<float> / 2.F),
          glm::vec3{1.F, 0.F, 0.F});
}
void BoundedApp::move(float polar, float azimuthal) {
  this->polar_ += polar;
  this->azimuthal_ += azimuthal;
  this->update_pos_and_rot();
  this->virtual_object_->commit();
}

namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void ack_configure(
    wl_client* /*client*/, wl_resource* resource, uint32_t serial) {
  auto* self = static_cast<util::UniPtr<BoundedApp>*>(
      wl_resource_get_user_data(resource));
  (*self)->ack_configure(serial);
}
void set_title(
    wl_client* /*client*/, wl_resource* /*resource*/, const char* /*title*/) {
  // TODO
}
void set_region(wl_client* /*client*/, wl_resource* resource,
    wl_resource* region_resource) {
  auto* self = static_cast<util::UniPtr<BoundedApp>*>(
      wl_resource_get_user_data(resource));
  auto* region = static_cast<util::UniPtr<region::Region>*>(
      wl_resource_get_user_data(region_resource));
  (*self)->set_region(region);
}
void move(wl_client* client, wl_resource* /*resource*/, wl_resource* /*seat*/,
    uint32_t /*serial*/) {
  server::get().seat->request_start_move(client);
}
constexpr struct zwn_bounded_interface kImpl = {
    .destroy       = destroy,
    .ack_configure = ack_configure,
    .set_title     = set_title,
    .set_region    = set_region,
    .move          = move,
};
void destroy(wl_resource* resource) {
  auto* self = static_cast<util::UniPtr<BoundedApp>*>(
      wl_resource_get_user_data(resource));
  delete self;
}
}  // namespace

void create(wl_client* client, uint32_t id, wl_array* half_size,
    zwin::virtual_object::VirtualObject* virtual_object) {
  wl_resource* resource =
      wl_resource_create(client, &zwn_bounded_interface, 1, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  auto app = std::make_shared<BoundedApp>(resource, virtual_object);
  app->configure(half_size);
  auto  obj  = std::dynamic_pointer_cast<input::BoundedObject>(std::move(app));
  auto* self = new util::UniPtr<input::BoundedObject>(std::move(obj));

  wl_resource_set_implementation(resource, &kImpl, self, destroy);
  server::get().bounded_apps.emplace_back(self->weak());
  virtual_object->set_app(self);
}
}  // namespace yaza::zwin::bounded
