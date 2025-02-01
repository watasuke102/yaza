#pragma once

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-util.h>

#include <cassert>
#include <csignal>

#include "common.hpp"
#include "input/bounded_object.hpp"
#include "input/seat.hpp"
#include "remote/remote.hpp"
#include "util/weakable_unique_ptr.hpp"

namespace yaza::server {
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

  remote::Remote*                                remote;
  input::Seat*                                   seat;
  std::list<util::WeakPtr<input::BoundedObject>> surfaces;
  void                                           remove_expired_surfaces();

 private:
  Server()  = default;
  ~Server() = default;
  static Server instance;

  bool        is_initialized_ = false;
  bool        is_started_     = false;
  bool        is_terminated_  = false;
  const char* socket_;

  wl_display*      wl_display_     = nullptr;
  wl_event_source* sigterm_source_ = nullptr;
  wl_event_source* sigquit_source_ = nullptr;
  wl_event_source* sigint_source_  = nullptr;
};

/// shortening
inline Server& get() {
  return Server::get_instance();
}
}  // namespace yaza::server
// NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)
