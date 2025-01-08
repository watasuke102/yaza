#include "zwin/seat.hpp"

#include <wayland-server-core.h>
#include <zwin-protocol.h>

#include <cstdint>

#include "server.hpp"

namespace yaza::zwin::seat {
namespace {
void get_ray(
    wl_client* /*client*/, wl_resource* /*resource*/, uint32_t /*id*/) {
  // TODO
}
void release(wl_client* /*client*/, wl_resource* /*resource*/) {
  // TODO
}
const struct zwn_seat_interface kImpl = {
    .get_ray = get_ray,
    .release = release,
};
}  // namespace

void bind(wl_client* client, void* data, uint32_t version, uint32_t id) {
  auto* server = static_cast<yaza::Server*>(data);

  wl_resource* resource = wl_resource_create(
      client, &zwn_seat_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, server, nullptr);
}
}  // namespace yaza::zwin::seat
