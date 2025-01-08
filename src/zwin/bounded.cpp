#include "zwin/bounded.hpp"

#include <wayland-server-core.h>
#include <zwin-shell-protocol.h>

#include <cstdint>

namespace yaza::zwin::bounded {
namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void ack_configure(
    wl_client* /*client*/, wl_resource* /*resource*/, uint32_t /*serial*/) {
  // TODO
}
void set_title(
    wl_client* /*client*/, wl_resource* /*resource*/, const char* /*title*/) {
  // TODO
}
void set_region(
    wl_client* /*client*/, wl_resource* /*resource*/, wl_resource* /*region*/) {
  // TODO
}
void move(wl_client* /*client*/, wl_resource* /*resource*/,
    wl_resource* /*seat*/, uint32_t /*serial*/) {
  // TODO
}
const struct zwn_bounded_interface kImpl = {
    .destroy       = destroy,
    .ack_configure = ack_configure,
    .set_title     = set_title,
    .set_region    = set_region,
    .move          = move,
};
}  // namespace

void create(wl_client* client, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &zwn_bounded_interface, 1, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yaza::zwin::bounded
