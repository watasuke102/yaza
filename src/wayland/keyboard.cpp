#include "wayland/keyboard.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>
#include <wayland-util.h>

#include "common.hpp"
#include "server.hpp"

namespace yaza::wayland::keyboard {
namespace {
void release(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
constexpr struct wl_keyboard_interface kImpl = {
    .release = release,
};

void destroy(wl_resource* resource) {
  wl_list_remove(wl_resource_get_link(resource));
}
}  // namespace

void create(wl_client* client, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_keyboard_interface, wl_keyboard_interface.version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, destroy);
  server::get().seat->client_seats[client]->add_keyboard(resource);
  LOG_DEBUG("created: wl_keyboard@%d for client %p", id, (void*)client);
}
}  // namespace yaza::wayland::keyboard
