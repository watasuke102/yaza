#include "wayland/data_device/data_device_manager.hpp"

#include <wayland-server-protocol.h>

#include "server.hpp"
#include "wayland/data_device/data_device.hpp"
#include "wayland/data_device/data_source.hpp"

namespace yaza::wayland::data_device_manager {
namespace {
void create_data_source(
    wl_client* client, wl_resource* /*resource*/, uint32_t id) {
  data_source::create(client, id);
}
void get_data_device(wl_client* client, wl_resource* /*resource*/, uint32_t id,
    wl_resource* /*seat*/) {
  auto* device = data_device::create(client, id);
  if (device && server::get().seat->is_focused_client(client)) {
    device->send_selection();
  }
}
constexpr struct wl_data_device_manager_interface kImpl = {
    .create_data_source = create_data_source,
    .get_data_device    = get_data_device,
};
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_data_device_manager_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yaza::wayland::data_device_manager
