#include "xdg_shell/xdg_toplevel.hpp"

#include <wayland-server-protocol.h>

#include "common.hpp"
#include "wayland-server-core.h"
#include "xdg-shell-protocol.h"

namespace yaza::xdg_shell::xdg_toplevel {
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
void move(wl_client* /*client*/, wl_resource* /*resource*/,
    wl_resource* /*seat*/, uint32_t /*serial*/) {
  // TODO
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

const struct xdg_toplevel_interface kImpl = {
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

}  // namespace

void create(wl_client* client, int version, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &xdg_toplevel_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
  LOG_DEBUG("xdg toplevel is created (client: %p, id: %u)", (void*)client, id);
}
}  // namespace yaza::xdg_shell::xdg_toplevel
