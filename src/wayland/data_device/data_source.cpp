#include "wayland/data_device/data_source.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include <utility>

namespace yaza::wayland::data_source {
DataSrc::DataSrc(wl_resource* resource) : resource_(resource) {
}
void DataSrc::add_mime_type(std::string&& offer) {
  this->mime_types_.emplace(std::move(offer));
}
void DataSrc::foreach_mime_types(
    std::function<void(const std::string&)>& handler) {
  for (const auto& e : this->mime_types_) {
    handler(e);
  }
}

namespace {
void offer(
    wl_client* /*client*/, wl_resource* resource, const char* mime_type) {
  get(resource)->add_mime_type(mime_type);
}
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void set_actions(wl_client* /*client*/, wl_resource* /*resource*/,
    uint32_t /*dnd_actions*/) {
  // TODO
}
constexpr struct wl_data_source_interface kImpl = {
    .offer       = offer,
    .destroy     = destroy,
    .set_actions = set_actions,
};

void destroy(wl_resource* resource) {
  delete get(resource);
}
}  // namespace

void create(wl_client* client, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_data_source_interface, wl_data_source_interface.version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  auto* self = new DataSrc(resource);
  wl_resource_set_implementation(resource, &kImpl, self, destroy);
}
DataSrc* get(wl_resource* resource) {
  return static_cast<DataSrc*>(wl_resource_get_user_data(resource));
}
}  // namespace yaza::wayland::data_source
