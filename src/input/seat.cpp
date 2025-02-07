#include "input/seat.hpp"

#include <GLES3/gl32.h>
#include <linux/input-event-codes.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>
#include <wayland-util.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <optional>

#include "common.hpp"
#include "input/bounded_object.hpp"
#include "input/input_listen_server.hpp"
#include "input/ray_caster.hpp"
#include "server.hpp"
#include "util/weakable_unique_ptr.hpp"
#include "wayland/data_device/data_device.hpp"
#include "wayland/surface.hpp"
#include "zwin/bounded.hpp"

namespace yaza::input {
bool Seat::is_focused_client(wl_client* client) {
  if (auto* focused_obj = this->keyboard_focused_surface_.lock()) {
    return client == focused_obj->client();
  }
  return false;
}

void Seat::set_keyboard_focused_surface(
    util::WeakPtr<input::BoundedObject> obj) {
  if (obj == this->keyboard_focused_surface_) {
    return;
  }
  this->try_leave_keyboard();

  auto     index  = this->keyboard_resources.bucket(obj->client());
  auto     serial = server::get().next_serial();
  wl_array keys;
  wl_array_init(&keys);
  std::for_each(this->keyboard_resources.begin(index),
      this->keyboard_resources.end(index),
      [serial, &obj, &keys](std::pair<wl_client*, wl_resource*> e) {
        wl_keyboard_send_enter(e.second, serial, obj->resource(), &keys);
      });
  wl_array_release(&keys);

  index = this->data_device_resources.bucket(obj->client());
  std::for_each(this->data_device_resources.begin(index),
      this->data_device_resources.end(index),
      [](std::pair<wl_client*, wayland::data_device::DataDevice*> e) {
        e.second->send_selection();
      });

  this->keyboard_focused_surface_ = std::move(obj);
}
void Seat::try_leave_keyboard() {
  auto* surface = dynamic_cast<wayland::surface::Surface*>(
      this->keyboard_focused_surface_.lock());
  if (!surface) {
    return;
  }
  auto index  = this->keyboard_resources.bucket(surface->client());
  auto serial = server::get().next_serial();
  std::for_each(this->keyboard_resources.begin(index),
      this->keyboard_resources.end(index),
      [surface, serial](std::pair<wl_client*, wl_resource*> e) {
        wl_keyboard_send_leave(e.second, serial, surface->resource());
      });
  this->keyboard_focused_surface_.reset();
}

void Seat::set_surface_as_cursor(
    wl_resource* surface_resource, int32_t hotspot_x, int32_t hotspot_y) {
  this->hotspot_.x = hotspot_x;
  this->hotspot_.y = hotspot_y;

  if (this->cursor_.lock()) {
    if (this->cursor_->resource() == surface_resource) {
      return;
    }
    {
      auto* cursor =
          dynamic_cast<wayland::surface::Surface*>(this->cursor_.lock());
      cursor->set_active(false);
    }
    server::get().surfaces.push_front(std::move(this->cursor_));
  }
  assert(this->cursor_.lock() == nullptr);

  server::get().remove_expired_surfaces();
  auto& surfaces = server::get().surfaces;
  auto  it       = std::find_if(surfaces.begin(), surfaces.end(),
             [surface_resource](util::WeakPtr<input::BoundedObject>& s) {
        return s->resource() == surface_resource;
      });

  if (it != surfaces.end()) {
    this->cursor_ = *it;
    surfaces.erase(it);
    assert(this->cursor_.lock() != nullptr);
    auto* cursor =
        dynamic_cast<wayland::surface::Surface*>(this->cursor_.lock());
    cursor->set_role(wayland::surface::Role::CURSOR);
    cursor->set_active(true);
    this->move_cursor();
  }
}
void Seat::move_cursor() {
  const auto pos =
      RayCaster::kOrigin + this->cursor_distance_ * this->ray_.direction();
  auto* cursor = dynamic_cast<wayland::surface::Surface*>(this->cursor_.lock());
  cursor->move(pos, this->ray_.rot(), this->hotspot_);
}

void Seat::handle_mouse_button(uint32_t button, wl_pointer_button_state state) {
  if (this->obj_state_ == FocusedObjState::MOVING &&
      state == WL_POINTER_BUTTON_STATE_RELEASED) {
    this->obj_state_ = FocusedObjState::DEFAULT;
    this->focused_obj_.reset();
    check_intersection();
    return;
  }

  if (auto* obj = this->focused_obj_.lock()) {
    obj->button(button, state);
    obj->frame();
  } else if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
    this->try_leave_keyboard();
  }
}
void Seat::handle_mouse_wheel(float amount) {
  if (auto* obj = this->focused_obj_.lock()) {
    obj->axis(amount);
    obj->frame();
  }
}

void Seat::request_start_move(wl_client* client) {
  auto* obj = this->focused_obj_.lock();
  if (!obj) {
    return;
  }
  if (obj->client() != client || !obj->is_active()) {
    return;
  }
  obj->leave();
  obj->frame();
  this->obj_state_ = FocusedObjState::MOVING;
}

void Seat::move_rel_pointing(float polar, float azimuthal) {
  const float diff_polar = this->ray_.move_rel(polar, azimuthal);
  switch (this->obj_state_) {
    case FocusedObjState::DEFAULT:
      this->check_intersection();
      break;
    case FocusedObjState::MOVING:
      if (auto* obj = this->focused_obj_.lock()) {
        obj->move(diff_polar, azimuthal);
      }
      break;
  }

  if (this->cursor_.lock()) {
    this->move_cursor();
  }
}
void Seat::check_intersection() {
  util::WeakPtr<BoundedObject> nearest_obj;
  std::optional<IntersectInfo> nearest_obj_info = std::nullopt;

  auto check = [direction = this->ray_.direction(), &nearest_obj,
                   &nearest_obj_info](auto&& objs) {
    for (auto it = objs.begin(); it != objs.end();) {
      if (auto* surface = it->lock()) {
        auto result =
            surface->check_intersection(RayCaster::kOrigin, direction);
        if (!result.has_value()) {
          ++it;
          continue;
        }
        if (!nearest_obj_info.has_value() ||
            result->distance <= nearest_obj_info->distance) {
          nearest_obj      = *it;
          nearest_obj_info = result.value();
        }
        ++it;
      } else {
        it = objs.erase(it);
      }
    }
  };
  check(server::get().surfaces);
  check(server::get().bounded_apps);

  if (nearest_obj_info.has_value()) {
    if (!nearest_obj->is_active()) {
      LOG_WARN("intersected obj is inactive (distance: %f)",
          nearest_obj_info->distance);
      return;  // FIXME: is this error handling correct?
    }
    bool focus_changed = this->set_focused_obj(nearest_obj);
    if (focus_changed) {
      this->focused_obj_->enter(nearest_obj_info.value());
    }
    this->focused_obj_->motion(nearest_obj_info.value());
    this->focused_obj_->frame();
    // show the cursor nearer to origin than hit position
    this->cursor_distance_ = nearest_obj_info->distance * 0.98F;
    this->ray_.set_length(this->cursor_distance_ * 0.9F);
  } else {
    // there is no intersected obj
    if (auto* obj = this->focused_obj_.lock()) {
      obj->leave();
      obj->frame();
    }
    this->focused_obj_.reset();
    this->set_surface_as_cursor(nullptr, 0, 0);
  }
}

bool Seat::set_focused_obj(util::WeakPtr<input::BoundedObject> obj) {
  if (obj == this->focused_obj_) {
    return false;
  }
  if (auto* obj = this->focused_obj_.lock()) {
    obj->leave();
    obj->frame();
  }
  if (dynamic_cast<zwin::bounded::BoundedApp*>(obj.lock())) {
    auto index = this->data_device_resources.bucket(obj->client());
    LOG_INFO("Focus on BoundedApp! -> %lu", index);
    std::for_each(this->data_device_resources.begin(index),
        this->data_device_resources.end(index),
        [](std::pair<wl_client*, wayland::data_device::DataDevice*> e) {
          e.second->send_selection();
        });
  }
  this->focused_obj_.swap(obj);
  return true;
}

}  // namespace yaza::input
