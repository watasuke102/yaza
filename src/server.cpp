#include "server.hpp"

#include <wayland-server-core.h>
#include <xdg-shell-protocol.h>

#include <cstdint>
#include <exception>

#include "common.hpp"
#include "remote/remote.hpp"
#include "wayland/wayland.hpp"
#include "xdg_shell/xdg_shell.hpp"
#include "zwin/zwin.hpp"

namespace yaza {
Server::Server() : is_started_(false) {
  this->wl_display_ = wl_display_create();
  if (!this->wl_display_) {
    LOG_ERR("Failed to create display");
    goto err;
  }
  wl_display_init_shm(this->wl_display_);

  this->socket_ = wl_display_add_socket_auto(this->wl_display_);
  setenv("WAYLAND_DISPLAY", this->socket_, true);

  remote::init(this->loop());

  if (!wayland::init(this->wl_display_, this)) {
    goto err_display;
  }
  if (!xdg_shell::init(this->wl_display_, this)) {
    goto err_display;
  }
  if (!zwin::init(this->wl_display_, this)) {
    goto err_display;
  }

  return;
err_display:
  wl_display_destroy(this->wl_display_);
  this->wl_display_ = nullptr;
err:
  throw std::exception();
}
Server::~Server() {
  LOG_INFO("destroying Server");
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
