#include "wayland/keyboard.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include "common.hpp"
#include "server.hpp"

namespace yaza::wayland::keyboard {
namespace {
void destroy(wl_resource* resource);

void release(wl_client* /*client*/, wl_resource* resource) {
  destroy(resource);
  wl_resource_destroy(resource);
}
constexpr struct wl_keyboard_interface kImpl = {
    .release = release,
};

void destroy(wl_resource* resource) {
  auto& keyboards = server::get().seat->keyboard_resources;
  for (auto it = keyboards.begin(); it != keyboards.end(); ++it) {
    if (it->second == resource) {
      keyboards.erase(it);
      return;
    }
  }
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
  server::get().seat->keyboard_resources.emplace(client, resource);
  LOG_DEBUG("created: wl_keyboard@%d for client %p", id, (void*)client);
}
}  // namespace yaza::wayland::keyboard
