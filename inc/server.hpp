#pragma once

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-util.h>

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

 private:
  bool        is_started_;
  const char* socket_;

  wl_display* wl_display_;
};
}  // namespace yaza
