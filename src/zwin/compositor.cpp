#include "zwin/compositor.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <zwin-protocol.h>

#include <cstdint>

#include "server.hpp"
#include "zwin/virtual_object.hpp"

namespace yaza::zwin::compositor {
namespace {
void create_virtual_object(
    wl_client* client, wl_resource* /*resource*/, uint32_t id) {
  virtual_object::create(client, id);
}
void create_region(
    wl_client* /*client*/, wl_resource* /*resource*/, uint32_t /*id*/) {
  // TODO
}
const struct zwn_compositor_interface kImpl = {
    .create_virtual_object = create_virtual_object,
    .create_region         = create_region,
};
}  // namespace

void bind(wl_client* client, void* data, uint32_t version, uint32_t id) {
  auto* server = static_cast<yaza::Server*>(data);

  wl_resource* resource = wl_resource_create(
      client, &zwn_compositor_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, server, nullptr);
}
}  // namespace yaza::zwin::compositor
