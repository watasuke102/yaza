#pragma once

#include <wayland-server-core.h>
#include <wayland-server.h>

#include <cstdint>

#include "common.hpp"
#include "wayland/data_device/data_source.hpp"

namespace yaza::wayland::data_device {
class DataDevice {
 public:
  DISABLE_MOVE_AND_COPY(DataDevice);
  explicit DataDevice(wl_resource* resource);
  ~DataDevice();

  wl_resource* resource() {
    return this->resource_;
  }
  data_source::DataSrc* data_source() {
    return this->data_source_;
  }

  void set_selection(data_source::DataSrc* source, uint32_t serial);
  void send_selection();

 private:
  wl_resource*          resource_;
  data_source::DataSrc* data_source_ = nullptr;
  uint32_t              serial_;
};

DataDevice* create(wl_client* client, uint32_t id);
DataDevice* get(wl_resource* resource);
}  // namespace yaza::wayland::data_device
