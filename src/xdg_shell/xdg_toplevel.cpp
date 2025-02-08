#include "xdg_shell/xdg_toplevel.hpp"

#include <wayland-server-protocol.h>
#include <wayland-util.h>

#include <cstdint>

#include "server.hpp"
#include "wayland-server-core.h"
#include "xdg-shell-protocol.h"
#include "xdg_shell/xdg_surface.hpp"

namespace yaza::xdg_shell::xdg_toplevel {
XdgTopLevel::XdgTopLevel(
    wl_resource* resource, xdg_surface::XdgSurface* surface)
    : xdg_surface_(surface), resource_(resource) {
}
void XdgTopLevel::set_activated(bool activated) {
  wl_array array;
  wl_array_init(&array);
  if (activated) {
    auto* e = static_cast<uint32_t*>(wl_array_add(&array, sizeof(uint32_t)));
    *e      = XDG_TOPLEVEL_STATE_ACTIVATED;
  }
  xdg_toplevel_send_configure(this->resource_, 0, 0, &array);
  wl_array_release(&array);
  this->xdg_surface_->send_configure();
}

namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void set_parent(
    wl_client* /*client*/, wl_resource* /*resource*/, wl_resource* /*parent*/) {
  // TODO
}
void set_title(
    wl_client* /*client*/, wl_resource* /*resource*/, const char* /*title*/) {
  // TODO
}
void set_app_id(
    wl_client* /*client*/, wl_resource* /*resource*/, const char* /*app_id*/) {
  // TODO
}
void show_window_menu(wl_client* /*client*/, wl_resource* /*resource*/,
    wl_resource* /*seat*/, uint32_t /*serial*/, int32_t /*x*/, int32_t /*y*/) {
  // TODO
}
void move(wl_client* client, wl_resource* /*resource*/, wl_resource* /*seat*/,
    uint32_t /*serial*/) {
  server::get().seat->request_start_move(client);
}
void resize(wl_client* /*client*/, wl_resource* /*resource*/,
    wl_resource* /*seat*/, uint32_t /*serial*/, uint32_t /*edges*/) {
  // TODO
}
void set_max_size(wl_client* /*client*/, wl_resource* /*resource*/,
    int32_t /*width*/, int32_t /*height*/) {
  // TODO
}
void set_min_size(wl_client* /*client*/, wl_resource* /*resource*/,
    int32_t /*width*/, int32_t /*height*/) {
  // TODO
}
void set_maximized(wl_client* /*client*/, wl_resource* /*resource*/) {
  // TODO
}
void unset_maximized(wl_client* /*client*/, wl_resource* /*resource*/) {
  // TODO
}
void set_fullscreen(
    wl_client* /*client*/, wl_resource* /*resource*/, wl_resource* /*output*/) {
  // TODO
}
void unset_fullscreen(wl_client* /*client*/, wl_resource* /*resource*/) {
  // TODO
}
void set_minimized(wl_client* /*client*/, wl_resource* /*resource*/) {
  // TODO
}

constexpr struct xdg_toplevel_interface kImpl = {
    .destroy          = destroy,
    .set_parent       = set_parent,
    .set_title        = set_title,
    .set_app_id       = set_app_id,
    .show_window_menu = show_window_menu,
    .move             = move,
    .resize           = resize,
    .set_max_size     = set_max_size,
    .set_min_size     = set_min_size,
    .set_maximized    = set_maximized,
    .unset_maximized  = unset_maximized,
    .set_fullscreen   = set_fullscreen,
    .unset_fullscreen = unset_fullscreen,
    .set_minimized    = set_minimized,
};

void destroy(wl_resource* resource) {
  auto* surface =
      static_cast<XdgTopLevel*>(wl_resource_get_user_data(resource));
  delete surface;
}
}  // namespace

XdgTopLevel* create(wl_client* client, int version, uint32_t id,
    xdg_surface::XdgSurface* surface) {
  wl_resource* resource =
      wl_resource_create(client, &xdg_toplevel_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return nullptr;
  }
  auto* self = new XdgTopLevel(resource, surface);
  wl_resource_set_implementation(resource, &kImpl, self, destroy);
  return self;
}
}  // namespace yaza::xdg_shell::xdg_toplevel
