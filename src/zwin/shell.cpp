#include "zwin/shell.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <zwin-shell-protocol.h>

#include <cstdint>

#include "common.hpp"
#include "server.hpp"
#include "zwin/bounded.hpp"

namespace yaza::zwin::shell {
namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void get_bounded(wl_client* client, wl_resource* resource, uint32_t id,
    wl_resource* /*virtual_object*/, wl_array* half_size) {
  auto* server =
      static_cast<yaza::Server*>(wl_resource_get_user_data(resource));
  auto* bounded_resource = bounded::create(client, id);
  if (bounded_resource) {
    zwn_bounded_send_configure(
        bounded_resource, half_size, server->next_serial());
  }
}
void get_expansive(wl_client* client, wl_resource* /*resource*/,
    uint32_t /*id*/, wl_resource* /*virtual_object*/) {
  // TODO
  wl_client_post_implementation_error(
      client, "expansive app is not yet supported");
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
