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
  void start();

 private:
  bool        is_started_;
  wl_display* wl_display_;
  const char* socket_;
};
}  // namespace yaza
