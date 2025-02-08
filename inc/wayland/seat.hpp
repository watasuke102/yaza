#pragma once

#include <wayland-server-core.h>
#include <wayland-server.h>

#include <functional>

#include "common.hpp"

namespace yaza::wayland::seat {
class ClientSeat {
 public:
  DISABLE_MOVE_AND_COPY(ClientSeat);
  explicit ClientSeat(wl_resource* resource);
  ~ClientSeat();

  void add_pointer(wl_resource* resource);
  void add_keyboard(wl_resource* resource);
  void add_data_device(wl_resource* resource);
  void add_ray(wl_resource* resource);

  void pointer_foreach(const std::function<void(wl_resource*)>& handler) const;
  void keyboard_foreach(const std::function<void(wl_resource*)>& handler) const;
  void data_device_foreach(
      const std::function<void(wl_resource*)>& handler) const;
  void ray_foreach(const std::function<void(wl_resource*)>& handler) const;

 private:
  wl_list pointers_;      // wl_pointer
  wl_list keyboards_;     // wl_keyboard
  wl_list data_devices_;  // wl_data_device
  wl_list rays_;          // zwn_ray

  wl_resource* resource_;
};

void        bind(wl_client* client, void* data, uint32_t version, uint32_t id);
ClientSeat* get(wl_resource* resource);
}  // namespace yaza::wayland::seat
