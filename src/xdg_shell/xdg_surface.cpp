#include "xdg_shell/xdg_surface.hpp"

#include <wayland-server-protocol.h>
#include <wayland-server.h>
#include <xdg-shell-protocol.h>

#include "server.hpp"
#include "util/weakable_unique_ptr.hpp"
#include "wayland/surface.hpp"
#include "xdg_shell/xdg_popup.hpp"
#include "xdg_shell/xdg_toplevel.hpp"

namespace yaza::xdg_shell::xdg_surface {
XdgSurface::XdgSurface(uint32_t id, wl_resource* resource,
    util::WeakPtr<wayland::surface::Surface>&& surface)
    : wl_surface_(std::move(surface)), resource_(resource), id_(id) {
  this->wl_surface_committed_listener_.set_handler(
      [this](std::nullptr_t* /*data*/) {
        if (this->is_first_commit_) {
          this->is_first_commit_ = false;
          this->send_configure();
        }
      });
  this->wl_surface_.lock()->listen_committed(
      this->wl_surface_committed_listener_);
  LOG_DEBUG("constructor: xdg_surface@%u", this->id_);
}
XdgSurface::~XdgSurface() {
  LOG_DEBUG(" destructor: xdg_surface@%u", this->id_);
}

void XdgSurface::set_wl_surface_role(
    wayland::surface::Role role, wayland::surface::RoleObject obj) {
  if (auto* wl_surface = this->wl_surface_.lock()) {
    wl_surface->set_role(role, obj);
  }
}
void XdgSurface::send_configure() {
  xdg_surface_send_configure(this->resource_, server::get().next_serial());
}

namespace {
void destroy(wl_client* /* client */, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void get_toplevel(wl_client* client, wl_resource* resource, uint32_t id) {
  auto* surface = static_cast<XdgSurface*>(wl_resource_get_user_data(resource));
  if (auto* toplevel = xdg_toplevel::create(
          client, wl_resource_get_version(resource), id, surface)) {
    surface->set_wl_surface_role(
        wayland::surface::Role::XDG_TOPLEVEL, toplevel);
  }
}
void get_popup(wl_client* client, wl_resource* /* resource */, uint32_t id,
    wl_resource* /* parent */, wl_resource* /* positioner */) {
  xdg_popup::create(client, id);
}
void set_window_geometry(wl_client* /* client */, wl_resource* /* resource */,
    int32_t /* x */, int32_t /* y */, int32_t /* width */,
    int32_t /* height */) {
  // TODO
}
void ack_configure(wl_client* /* client */, wl_resource* /* resource */,
    uint32_t /* serial */) {
  // TODO
}
constexpr struct xdg_surface_interface kImpl = {
    .destroy             = destroy,
    .get_toplevel        = get_toplevel,
    .get_popup           = get_popup,
    .set_window_geometry = set_window_geometry,
    .ack_configure       = ack_configure,
};

void destroy(wl_resource* resource) {
  auto* surface = static_cast<XdgSurface*>(wl_resource_get_user_data(resource));
  delete surface;
}
}  // namespace

void create(wl_client* client, int version, uint32_t id,
    util::WeakPtr<wayland::surface::Surface>&& surface) {
  wl_resource* resource =
      wl_resource_create(client, &xdg_surface_interface, version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  auto* xdg_surface = new XdgSurface(id, resource, std::move(surface));
  wl_resource_set_implementation(resource, &kImpl, xdg_surface, destroy);
}
}  // namespace yaza::xdg_shell::xdg_surface
