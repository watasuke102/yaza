#pragma once

#include <wayland-server-core.h>
#include <zen-remote/loop.h>

#include "common.hpp"

namespace yaza::remote {
class Loop : public zen::remote::ILoop {
 public:
  DISABLE_MOVE_AND_COPY(Loop);
  explicit Loop(wl_event_loop* wl_loop);
  ~Loop() override = default;
  void AddFd(zen::remote::FdSource* source) override;
  void RemoveFd(zen::remote::FdSource* source) override;
  void Terminate() override;

 private:
  wl_event_loop* wl_loop_;
};
}  // namespace yaza::remote
