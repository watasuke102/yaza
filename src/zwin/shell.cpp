#include "zwin/shell.hpp"

#include <wayland-server-core.h>
#include <zwin-shell-protocol.h>

#include <cstdint>

#include "server.hpp"

namespace yaza::zwin::shell {
namespace {
void destroy(wl_client* /*client*/, wl_resource* /*resource*/) {
  // TODO
}
void get_bounded(wl_client* /*client*/, wl_resource* /*resource*/,
    uint32_t /*id*/, wl_resource* /*virtual_object*/, wl_array* /*half_size*/) {
  // TODO
}
void get_expansive(wl_client* /*client*/, wl_resource* /*resource*/,
    uint32_t /*id*/, wl_resource* /*virtual_object*/) {
  // TODO
}
const struct zwn_shell_interface kImpl = {
    .destroy       = destroy,
    .get_bounded   = get_bounded,
    .get_expansive = get_expansive,
};
}  // namespace

void bind(wl_client* client, void* data, uint32_t version, uint32_t id) {
  auto* server = static_cast<yaza::Server*>(data);

  wl_resource* resource = wl_resource_create(
      client, &zwn_shell_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, server, nullptr);
}
}  // namespace yaza::zwin::shell
