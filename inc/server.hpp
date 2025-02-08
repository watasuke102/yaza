#pragma once

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>
#include <wayland-util.h>

#include <cassert>
#include <csignal>
#include <functional>
#include <optional>

#include "common.hpp"
#include "input/bounded_object.hpp"
#include "input/server_seat.hpp"
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

  remote::Remote*    remote;
  input::ServerSeat* seat;

  void add_surface(util::WeakPtr<input::BoundedObject>&& surface);
  void add_bounded_app(util::WeakPtr<input::BoundedObject>&& bounded_app);

  void foreach_surface(
      const std::function<void(util::WeakPtr<input::BoundedObject>&)>& handler);
  void foreach_bounded_app(
      const std::function<void(util::WeakPtr<input::BoundedObject>&)>& handler);

  std::optional<util::WeakPtr<input::BoundedObject>> get_surface_from_resource(
      wl_resource* resource);

 private:
  Server()  = default;
  ~Server() = default;
  static Server instance;

  std::list<util::WeakPtr<input::BoundedObject>> surfaces_;
  std::list<util::WeakPtr<input::BoundedObject>> bounded_apps_;

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
