#pragma once

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-util.h>

#include <csignal>

#include "common.hpp"

namespace yaza {
class Server {
 public:
  DISABLE_MOVE_AND_COPY(Server);
  Server();
  ~Server();
  void           start();
  uint32_t       next_serial();
  wl_event_loop* loop();

  [[nodiscard]] bool is_creation_failed() const {
    return this->is_creation_failed_;
  }

 private:
  bool        is_creation_failed_ = false;
  bool        is_started_         = false;
  const char* socket_;

  wl_display*      wl_display_     = nullptr;
  wl_event_source* sigterm_source_ = nullptr;
  wl_event_source* sigquit_source_ = nullptr;
  wl_event_source* sigint_source_  = nullptr;
};
}  // namespace yaza
