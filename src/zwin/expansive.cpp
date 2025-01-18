#include "zwin/expansive.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <zwin-shell-protocol.h>

#include <cstdint>

namespace yaza::zwin::expansive {
namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void set_region(
    wl_client* /*client*/, wl_resource* /*resource*/, wl_resource* /*region*/) {
  // TODO
}
const struct zwn_expansive_interface kImpl = {
    .destroy    = destroy,
    .set_region = set_region,
};
}  // namespace

wl_resource* create(wl_client* client, uint32_t id,
    virtual_object::VirtualObject* /*virtual_object*/) {
  wl_resource* resource =
      wl_resource_create(client, &zwn_expansive_interface, 1, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return nullptr;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
  return resource;
}
}  // namespace yaza::zwin::expansive
