#include "zwin/shell.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>
#include <zwin-shell-protocol.h>

#include <cstdint>

#include "server.hpp"
#include "zwin/bounded.hpp"
#include "zwin/expansive.hpp"
#include "zwin/virtual_object.hpp"

namespace yaza::zwin::shell {
namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void get_bounded(wl_client* client, wl_resource* /*resource*/, uint32_t id,
    wl_resource* /*virtual_object*/, wl_array* half_size) {
  auto* bounded_resource = bounded::create(client, id);
  if (bounded_resource) {
    zwn_bounded_send_configure(
        bounded_resource, half_size, server::get().next_serial());
  }
}
void get_expansive(wl_client* client, wl_resource* /*resource*/, uint32_t id,
    wl_resource* virtual_object_resource) {
  auto* virtual_object = static_cast<virtual_object::VirtualObject*>(
      wl_resource_get_user_data(virtual_object_resource));
  expansive::create(client, id, virtual_object);
}
const struct zwn_shell_interface kImpl = {
    .destroy       = destroy,
    .get_bounded   = get_bounded,
    .get_expansive = get_expansive,
};
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &zwn_shell_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yaza::zwin::shell
