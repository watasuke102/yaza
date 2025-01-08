#include "zwin/virtual_object.hpp"

#include <wayland-server-protocol.h>
#include <zwin-protocol.h>

namespace yaza::zwin::virtual_object {
namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void commit(wl_client* /*client*/, wl_resource* /*resource*/) {
  // TODO
}
void frame(
    wl_client* /*client*/, wl_resource* /*resource*/, uint32_t /*callback*/) {
  // TODO
}
const struct zwn_virtual_object_interface kImpl = {
    .destroy = destroy,
    .commit  = commit,
    .frame   = frame,
};
}  // namespace

void create(wl_client* client, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &zwn_virtual_object_interface, 1, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yaza::zwin::virtual_object
