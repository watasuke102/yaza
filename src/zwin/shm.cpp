#include "zwin/shm.hpp"

#include <wayland-server-core.h>
#include <zwin-protocol.h>

#include <cstdint>

#include "server.hpp"

namespace yaza::zwin::shm {
namespace {
void create_pool(wl_client* /*client*/, wl_resource* /*resource*/,
    uint32_t /*id*/, int32_t /*fd*/, wl_array* /*size*/) {
  // TODO
}
const struct zwn_shm_interface kImpl = {
    .create_pool = create_pool,
};
}  // namespace

void bind(wl_client* client, void* data, uint32_t version, uint32_t id) {
  auto* server = static_cast<yaza::Server*>(data);

  wl_resource* resource = wl_resource_create(
      client, &zwn_shm_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, server, nullptr);
}
}  // namespace yaza::zwin::shm
