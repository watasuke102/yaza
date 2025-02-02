#include "wayland/data_device/data_offer.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include <functional>
#include <string>

#include "wayland/data_device/data_source.hpp"

namespace yaza::wayland::data_offer {
DataOffer::DataOffer(wl_resource* resource, data_source::DataSrc* data_source)
    : resource_(resource), data_source_(data_source) {
}

namespace {
void accept(wl_client* /*client*/, wl_resource* resource, uint32_t /*serial*/,
    const char* mime_type) {
  wl_data_source_send_target(get(resource)->data_source(), mime_type);
}
void receive(wl_client* /*client*/, wl_resource* resource,
    const char* mime_type, int32_t fd) {
  wl_data_source_send_send(get(resource)->data_source(), mime_type, fd);
}
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void finish(wl_client* /*client*/, wl_resource* /*resource*/) {
  // TODO
}
void set_actions(wl_client* /*client*/, wl_resource* /*resource*/,
    uint32_t /*dnd_actions*/, uint32_t /*preferred_action*/) {
  // TODO
}
constexpr struct wl_data_offer_interface kImpl = {
    .accept      = accept,
    .receive     = receive,
    .destroy     = destroy,
    .finish      = finish,
    .set_actions = set_actions,
};

void destroy(wl_resource* resource) {
  delete get(resource);
}
}  // namespace

wl_resource* create(wl_client* client, data_source::DataSrc* data_source) {
  wl_resource* resource = wl_resource_create(
      client, &wl_data_offer_interface, wl_data_offer_interface.version, 0);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return nullptr;
  }
  auto* self = new DataOffer(resource, data_source);
  wl_resource_set_implementation(resource, &kImpl, self, destroy);

  std::function<void(const std::string&)> handler =
      [resource](const std::string& mime_type) {
        wl_data_offer_send_offer(resource, mime_type.c_str());
      };
  data_source->foreach_mime_types(handler);

  return resource;
}
DataOffer* get(wl_resource* resource) {
  return static_cast<DataOffer*>(wl_resource_get_user_data(resource));
}
}  // namespace yaza::wayland::data_offer
