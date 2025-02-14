#pragma once

#include <wayland-server.h>
#include <xdg-shell-protocol.h>

#include <cstdint>
#include <memory>

#include "common.hpp"

namespace yaza::xdg_shell::xdg_positioner {
struct PositionerGeometry {
  int32_t x = 0, y = 0;
  int32_t width, height;
};
struct PositionInfo;

class XdgPositioner {
 public:
  DISABLE_MOVE_AND_COPY(XdgPositioner);
  explicit XdgPositioner(wl_resource* resource);
  ~XdgPositioner() = default;

  std::unique_ptr<PositionInfo>    info;
  [[nodiscard]] PositionerGeometry geometry() const;

 private:
  wl_resource* resource_;
};

void           create(wl_client* client, uint32_t id);
XdgPositioner* get(wl_resource* resource);
}  // namespace yaza::xdg_shell::xdg_positioner
