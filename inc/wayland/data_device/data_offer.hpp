#pragma once

#include <wayland-server-core.h>
#include <wayland-server.h>

#include "wayland/data_device/data_device.hpp"
#include "wayland/data_device/data_source.hpp"

namespace yaza::wayland::data_offer {
class DataOffer {
 public:
  DISABLE_MOVE_AND_COPY(DataOffer);
  DataOffer(wl_resource* resource, data_source::DataSrc* data_source);
  ~DataOffer() = default;

  wl_resource* resource() {
    return this->resource_;
  }
  wl_resource* data_source() {
    return this->data_source_->resource();
  }

 private:
  wl_resource*          resource_;
  data_source::DataSrc* data_source_;
};

wl_resource* create(data_device::DataDevice* data_device);
DataOffer*   get(wl_resource* resource);
}  // namespace yaza::wayland::data_offer
