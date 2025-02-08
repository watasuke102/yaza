#include "server.hpp"

#include <unistd.h>
#include <wayland-server-core.h>
#include <xdg-shell-protocol.h>

#include <csignal>
#include <cstdint>
#include <optional>
#include <utility>

#include "common.hpp"
#include "input/bounded_object.hpp"
#include "input/server_seat.hpp"
#include "remote/remote.hpp"
#include "util/weakable_unique_ptr.hpp"
#include "wayland/surface.hpp"
#include "wayland/wayland.hpp"
#include "xdg_shell/xdg_shell.hpp"
#include "zwin/zwin.hpp"

namespace yaza::server {
namespace {
int handle_signal(int signum, void* /*data*/) {
  LOG_INFO("Signal received: %d", signum);
  get().terminate();
  return 0;
}
}  // namespace
Server Server::instance;

bool Server::init() {
  // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define BAIL(err)                                                              \
  if (err) {                                                                   \
    LOG_ERR(err);                                                              \
  }                                                                            \
  return false

  assert(!instance.is_initialized_);
  instance.is_initialized_ = true;

  instance.wl_display_ = wl_display_create();
  if (!instance.wl_display_) {
    BAIL("Failed to create display");
  }
  wl_display_init_shm(instance.wl_display_);

  instance.remote = new remote::Remote(instance.loop());
  instance.seat   = new input::ServerSeat();

  if (!wayland::init(instance.wl_display_)) {
    BAIL(nullptr);
  }
  if (!xdg_shell::init(instance.wl_display_)) {
    BAIL(nullptr);
  }
  if (!zwin::init(instance.wl_display_)) {
    BAIL(nullptr);
  }

  instance.sigterm_source_ = wl_event_loop_add_signal(
      instance.loop(), SIGTERM, handle_signal, instance.wl_display_);
  if (!instance.sigterm_source_) {
    BAIL("Failed to add SIGTERM handler");
  }
  instance.sigquit_source_ = wl_event_loop_add_signal(
      instance.loop(), SIGQUIT, handle_signal, instance.wl_display_);
  if (!instance.sigterm_source_) {
    BAIL("Failed to add SIGQUIT handler");
  }
  instance.sigint_source_ = wl_event_loop_add_signal(
      instance.loop(), SIGINT, handle_signal, instance.wl_display_);
  if (!instance.sigint_source_) {
    BAIL("Failed to add SIGINT handler");
  }

  instance.socket_ = wl_display_add_socket_auto(instance.wl_display_);
  setenv("WAYLAND_DISPLAY", instance.socket_, true);
  return true;
#undef BAIL
}
void Server::terminate() {
  assert(this->is_initialized_);
  if (this->is_terminated_) {
    return;
  }
  LOG_INFO("destroying Server");
  this->is_terminated_ = true;
  if (this->wl_display_) {
    wl_display_destroy_clients(this->wl_display_);
  }
  delete this->remote;
  if (this->sigint_source_) {
    wl_event_source_remove(sigint_source_);
  }
  if (this->sigquit_source_) {
    wl_event_source_remove(sigquit_source_);
  }
  if (this->sigterm_source_) {
    wl_event_source_remove(sigterm_source_);
  }
  delete this->seat;
  if (this->wl_display_) {
    wl_display_destroy(this->wl_display_);
  }
}

void Server::start() {
  if (this->is_started_) {
    return;
  }
  this->is_started_ = true;
  LOG_INFO("server started at %s", this->socket_);
  wl_display_run(this->wl_display_);
}
uint32_t Server::next_serial() {
  return wl_display_next_serial(this->wl_display_);
}
wl_event_loop* Server::loop() {
  return wl_display_get_event_loop(this->wl_display_);
}

void Server::add_surface(util::WeakPtr<input::BoundedObject>&& surface) {
  this->surfaces_.emplace_front(std::move(surface));
  this->reorder_surfaces();
}
void Server::add_bounded_app(
    util::WeakPtr<input::BoundedObject>&& bounded_app) {
  this->bounded_apps_.emplace_front(std::move(bounded_app));
}

void Server::foreach_surface(
    const std::function<void(util::WeakPtr<input::BoundedObject>&)>& handler) {
  for (auto it = this->surfaces_.begin(); it != this->surfaces_.end();) {
    if (it->lock()) {
      handler(*it);
      ++it;
    } else {
      it = this->surfaces_.erase(it);
    }
  }
}
void Server::foreach_bounded_app(
    const std::function<void(util::WeakPtr<input::BoundedObject>&)>& handler) {
  for (auto it = this->bounded_apps_.begin();
      it != this->bounded_apps_.end();) {
    if (it->lock()) {
      handler(*it);
      ++it;
    } else {
      it = this->bounded_apps_.erase(it);
    }
  }
}

std::optional<util::WeakPtr<input::BoundedObject>>
Server::get_surface_from_resource(wl_resource* wl_surface) {
  auto it = std::find_if(this->surfaces_.begin(), this->surfaces_.end(),
      [wl_surface](util::WeakPtr<input::BoundedObject>& s) {
        if (!s.lock()) {
          return false;
        }
        return s->resource() == wl_surface;
      });
  return it == this->surfaces_.end() ? std::nullopt : std::make_optional(*it);
}
void Server::raise_surface_top(wl_resource* wl_surface) {
  auto it = std::find_if(this->surfaces_.begin(), this->surfaces_.end(),
      [wl_surface](util::WeakPtr<input::BoundedObject>& s) {
        if (!s.lock()) {
          return false;
        }
        return s->resource() == wl_surface;
      });
  if (it != this->surfaces_.end()) {
    this->surfaces_.splice(this->surfaces_.begin(), this->surfaces_, it);
    this->reorder_surfaces();
  }
}

void Server::reorder_surfaces() {
  uint64_t surface_index = 0;
  this->foreach_surface(
      [&surface_index](util::WeakPtr<input::BoundedObject>& obj) {
        auto* surface = dynamic_cast<wayland::surface::Surface*>(obj.lock());
        if (surface->update_distance_by_layer_index(surface_index)) {
          ++surface_index;
        }
      });
}
}  // namespace yaza::server
