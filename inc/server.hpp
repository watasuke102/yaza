#pragma once

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-util.h>

#include <cassert>
#include <csignal>

#include "common.hpp"
#include "remote/remote.hpp"

namespace yaza {
namespace wayland::seat {
class Seat;
}
namespace server {
// NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
class Server {
 public:
  DISABLE_MOVE_AND_COPY(Server);
  /// return false if failed
  static bool    init();
  static Server& get_instance() {
    return instance;
  }
  void terminate();

  void           start();
  uint32_t       next_serial();
  wl_event_loop* loop();

  remote::Remote* remote;

 private:
  Server()  = default;
  ~Server() = default;
  static Server instance;

  bool        is_initialized_ = false;
  bool        is_started_     = false;
  bool        is_terminated_  = false;
  const char* socket_;

  wayland::seat::Seat* seat_           = nullptr;
  wl_display*          wl_display_     = nullptr;
  wl_event_source*     sigterm_source_ = nullptr;
  wl_event_source*     sigquit_source_ = nullptr;
  wl_event_source*     sigint_source_  = nullptr;
};

/// shortening
inline Server& get() {
  return Server::get_instance();
}
}  // namespace server
}  // namespace yaza
// NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)
