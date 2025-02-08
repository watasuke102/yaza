#include "wayland/data_device/data_device.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include "server.hpp"
#include "wayland/data_device/data_offer.hpp"
#include "wayland/data_device/data_source.hpp"

namespace yaza::wayland::data_device {
DataDevice::DataDevice(wl_resource* resource) : resource_(resource) {
}
void DataDevice::set_selection(data_source::DataSrc* source, uint32_t serial) {
  this->data_source_                    = source;
  this->serial_                         = serial;
  server::get().seat->current_selection = this;
}
void DataDevice::send_selection() {
  if (server::get().seat->current_selection == nullptr) {
    wl_data_device_send_selection(this->resource(), nullptr);
    return;
  }
  auto* offer = data_offer::create(this);
  if (offer != nullptr) {
    wl_data_device_send_selection(this->resource(), offer);
  }
}

namespace {
void start_drag(wl_client* /*client*/, wl_resource* /*resource*/,
    wl_resource* /*source*/, wl_resource* /*origin*/, wl_resource* /*icon*/,
    uint32_t /*serial*/) {
  // TODO
}
void set_selection(wl_client* /*client*/, wl_resource* resource,
    wl_resource* data_source_resource, uint32_t serial) {
  get(resource)->set_selection(data_source::get(data_source_resource), serial);
}
void release(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
constexpr struct wl_data_device_interface kImpl = {.start_drag = start_drag,
    .set_selection                                             = set_selection,
    .release                                                   = release};

void destroy(wl_resource* resource) {
  wl_list_remove(wl_resource_get_link(resource));
  auto* self = get(resource);
  delete self;
}
}  // namespace

DataDevice* create(wl_client* client, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_data_device_interface, wl_data_device_interface.version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return nullptr;
  }
  auto* self = new DataDevice(resource);
  wl_resource_set_implementation(resource, &kImpl, self, destroy);
  server::get().seat->client_seats[client]->add_data_device(resource);
  LOG_DEBUG("created: wl_data_device@%d for client %p", id, (void*)client);
  return self;
}
DataDevice* get(wl_resource* resource) {
  return static_cast<DataDevice*>(wl_resource_get_user_data(resource));
}
}  // namespace yaza::wayland::data_device
