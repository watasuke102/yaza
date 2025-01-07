#include "server.hpp"

#include <exception>

#include "common.hpp"
#include "compositor.hpp"
#include "xdg-shell-protocol.h"
#include "xdg_shell.hpp"

namespace yaza {
Server::Server() : is_started_(false) {
  this->wl_display_ = wl_display_create();
  if (!this->wl_display_) {
    LOG_ERR("Failed to create display");
    throw std::exception();
  }
  wl_display_init_shm(this->wl_display_);

  this->socket_ = wl_display_add_socket_auto(this->wl_display_);
  setenv("WAYLAND_DISPLAY", this->socket_, true);

  if (!wl_global_create(this->wl_display_, &wl_compositor_interface, 5, this,
          compositor::bind)) {
    LOG_ERR("Failed to create global (compositor)");
    throw std::exception();
  }
  if (!wl_global_create(this->wl_display_, &xdg_wm_base_interface, 1, this,
          xdg_shell::bind)) {
    LOG_ERR("Failed to create global (xdg_wm_base)");
    throw std::exception();
  }
}
Server::~Server() {
  wl_display_destroy_clients(this->wl_display_);
  wl_display_destroy(this->wl_display_);
}

void Server::start() {
  if (this->is_started_) {
    return;
  }
  this->is_started_ = true;
  LOG_INFO("server started at %s", this->socket_);
  wl_display_run(this->wl_display_);
}
}  // namespace yaza
