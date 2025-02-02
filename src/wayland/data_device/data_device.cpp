#include "wayland/data_device/data_device.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include "server.hpp"
#include "wayland/data_device/data_source.hpp"

namespace yaza::wayland::data_device {
namespace {
void start_drag(wl_client* /*client*/, wl_resource* /*resource*/,
    wl_resource* /*source*/, wl_resource* /*origin*/, wl_resource* /*icon*/,
    uint32_t /*serial*/) {
  // TODO
}
void set_selection(wl_client* /*client*/, wl_resource* /*resource*/,
    wl_resource* data_source_resource, uint32_t serial) {
  server::get().seat->selection = {
      .source = data_source::get(data_source_resource), .serial = serial};
}
void release(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
constexpr struct wl_data_device_interface kImpl = {.start_drag = start_drag,
    .set_selection                                             = set_selection,
    .release                                                   = release};

void destroy(wl_resource* resource) {
  auto& devices = server::get().seat->data_device_resources;
  for (auto it = devices.begin(); it != devices.end(); ++it) {
    if (it->second == resource) {
      devices.erase(it);
      return;
    }
  }
}
}  // namespace

wl_resource* create(wl_client* client, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_data_device_interface, wl_data_device_interface.version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return nullptr;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, destroy);
  server::get().seat->data_device_resources.emplace(client, resource);
  return resource;
}
}  // namespace yaza::wayland::data_device
