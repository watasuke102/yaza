#include "server.hpp"

#include <wayland-server-core.h>
#include <xdg-shell-protocol.h>
#include <zwin-gles-v32-protocol.h>
#include <zwin-protocol.h>
#include <zwin-shell-protocol.h>

#include <cstdint>
#include <exception>

#include "common.hpp"
#include "compositor.hpp"
#include "remote/remote.hpp"
#include "xdg_shell.hpp"
#include "zwin/compositor.hpp"
#include "zwin/gles_v32/gles_v32.hpp"
#include "zwin/seat.hpp"
#include "zwin/shell.hpp"
#include "zwin/shm/shm.hpp"

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

  this->compositor_ = wl_global_create(
      this->wl_display_, &wl_compositor_interface, 5, this, compositor::bind);
  if (!this->compositor_) {
    LOG_ERR("Failed to create global (compositor)");
    goto err_display;
  }

  this->xdg_wm_base_ = wl_global_create(
      this->wl_display_, &xdg_wm_base_interface, 1, this, xdg_shell::bind);
  if (!this->xdg_wm_base_) {
    LOG_ERR("Failed to create global (xdg_wm_base)");
    goto err_compositor;
  }

  this->zwin_compositor_ = wl_global_create(this->wl_display_,
      &zwn_compositor_interface, 1, this, zwin::compositor::bind);
  if (!this->zwin_compositor_) {
    LOG_ERR("Failed to create global (zwin_compositor)");
    goto err_xdg_wm_base;
  }

  this->zwin_seat_ = wl_global_create(
      this->wl_display_, &zwn_seat_interface, 1, this, zwin::seat::bind);
  if (!this->zwin_seat_) {
    LOG_ERR("Failed to create global (zwin_seat)");
    goto err_zwin_compositor;
  }

  this->zwin_shell_ = wl_global_create(
      this->wl_display_, &zwn_shell_interface, 1, this, zwin::shell::bind);
  if (!this->zwin_shell_) {
    LOG_ERR("Failed to create global (zwin_shell)");
    goto err_zwin_seat;
  }

  this->zwin_shm_ = wl_global_create(
      this->wl_display_, &zwn_shm_interface, 1, this, zwin::shm::bind);
  if (!this->zwin_shm_) {
    LOG_ERR("Failed to create global (zwin_shm)");
    goto err_zwin_shell;
  }

  this->zwin_gles_v32_ = wl_global_create(this->wl_display_,
      &zwn_gles_v32_interface, 1, this, zwin::gles_v32::bind);
  if (!this->zwin_gles_v32_) {
    LOG_ERR("Failed to create global (zwin_gles_v32)");
    goto err_zwin_shm;
  }

  return;
err_zwin_shm:
  wl_global_destroy(this->zwin_shm_);
  this->zwin_shm_ = nullptr;
err_zwin_shell:
  wl_global_destroy(this->zwin_shell_);
  this->zwin_shell_ = nullptr;
err_zwin_seat:
  wl_global_destroy(this->zwin_seat_);
  this->zwin_seat_ = nullptr;
err_zwin_compositor:
  wl_global_destroy(this->zwin_compositor_);
  this->zwin_compositor_ = nullptr;
err_xdg_wm_base:
  wl_global_destroy(this->xdg_wm_base_);
  this->xdg_wm_base_ = nullptr;
err_compositor:
  wl_global_destroy(this->compositor_);
  this->compositor_ = nullptr;
err_display:
  wl_display_destroy(this->wl_display_);
  this->wl_display_ = nullptr;
err:
  throw std::exception();
}
Server::~Server() {
  LOG_DEBUG("destroying Server");
  if (this->zwin_gles_v32_) {
    wl_global_destroy(zwin_gles_v32_);
  }
  if (this->zwin_shm_) {
    wl_global_destroy(zwin_shm_);
  }
  if (this->zwin_shell_) {
    wl_global_destroy(zwin_shell_);
  }
  if (this->zwin_seat_) {
    wl_global_destroy(zwin_seat_);
  }
  if (this->zwin_compositor_) {
    wl_global_destroy(zwin_compositor_);
  }
  if (this->xdg_wm_base_) {
    wl_global_destroy(this->xdg_wm_base_);
  }
  if (this->compositor_) {
    wl_global_destroy(this->compositor_);
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
