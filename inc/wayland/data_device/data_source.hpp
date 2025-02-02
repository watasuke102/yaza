#pragma once

#include <wayland-server-core.h>
#include <wayland-server.h>

#include <functional>
#include <string>
#include <unordered_set>

#include "common.hpp"

namespace yaza::wayland::data_source {
class DataSrc {
 public:
  DISABLE_MOVE_AND_COPY(DataSrc);
  explicit DataSrc(wl_resource* resource);
  ~DataSrc() = default;

  wl_resource* resource() {
    return this->resource_;
  }

  void add_mime_type(std::string&& offer);
  void foreach_mime_types(std::function<void(const std::string&)>& handler);

 private:
  std::unordered_set<std::string> mime_types_;
  wl_resource*                    resource_;
};

void     create(wl_client* client, uint32_t id);
DataSrc* get(wl_resource* resource);
}  // namespace yaza::wayland::data_source
