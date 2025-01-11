#include "server.hpp"

#include <unistd.h>
#include <wayland-server-core.h>
#include <xdg-shell-protocol.h>

#include <csignal>
#include <cstdint>

#include "common.hpp"
#include "remote/remote.hpp"
#include "wayland/wayland.hpp"
#include "xdg_shell/xdg_shell.hpp"
#include "zwin/zwin.hpp"

namespace yaza {
namespace {
int handle_signal(int /*signum*/, void* data) {
  auto* display = static_cast<wl_display*>(data);
  wl_display_terminate(display);
  return 0;
}
}  // namespace

Server::Server() {
  // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define BAIL(err)                                                              \
  if (err) {                                                                   \
    LOG_ERR(err);                                                              \
  }                                                                            \
  this->is_creation_failed_ = true;                                            \
  return

  this->wl_display_ = wl_display_create();
  if (!this->wl_display_) {
    BAIL("Failed to create display");
  }
  wl_display_init_shm(this->wl_display_);

  remote::init(this->loop());

  if (!wayland::init(this->wl_display_, this)) {
    BAIL(nullptr);
  }
  if (!xdg_shell::init(this->wl_display_, this)) {
    BAIL(nullptr);
  }
  if (!zwin::init(this->wl_display_, this)) {
    BAIL(nullptr);
  }

  this->sigterm_source_ = wl_event_loop_add_signal(
      this->loop(), SIGTERM, handle_signal, this->wl_display_);
  if (!this->sigterm_source_) {
    BAIL("Failed to add SIGTERM handler");
  }
  this->sigquit_source_ = wl_event_loop_add_signal(
      this->loop(), SIGQUIT, handle_signal, this->wl_display_);
  if (!this->sigterm_source_) {
    BAIL("Failed to add SIGQUIT handler");
  }
  this->sigint_source_ = wl_event_loop_add_signal(
      this->loop(), SIGINT, handle_signal, this->wl_display_);
  if (!this->sigint_source_) {
    BAIL("Failed to add SIGINT handler");
  }

  this->socket_ = wl_display_add_socket_auto(this->wl_display_);
  setenv("WAYLAND_DISPLAY", this->socket_, true);
#undef BAIL
}
Server::~Server() {
  LOG_INFO("destroying Server");
  remote::terminate();
  if (this->sigint_source_) {
    wl_event_source_remove(sigint_source_);
  }
  if (this->sigquit_source_) {
    wl_event_source_remove(sigquit_source_);
  }
  if (this->sigterm_source_) {
    wl_event_source_remove(sigterm_source_);
  }
  if (this->wl_display_) {
    wl_display_destroy_clients(this->wl_display_);
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
}  // namespace yaza
