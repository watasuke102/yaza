#include "xdg_shell/xdg_popup.hpp"

#include <xdg-shell-protocol.h>

namespace yaza::xdg_shell::xdg_popup {
namespace {
void destroy(wl_client* /*client*/, wl_resource* /*resource*/) {
  // TODO
}
void grab(wl_client* /*client*/, wl_resource* /*resource*/,
    wl_resource* /*seat*/, uint32_t /*serial*/) {
  // TODO
}
void reposition(wl_client* /*client*/, wl_resource* /*resource*/,
    wl_resource* /*positioner*/, uint32_t /*token*/) {
  // TODO
}
constexpr struct xdg_popup_interface kImpl = {
    .destroy    = destroy,
    .grab       = grab,
    .reposition = reposition,
};  // namespace
}  // namespace

void create(wl_client* client, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &xdg_popup_interface, xdg_popup_interface.version, id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
}
}  // namespace yaza::xdg_shell::xdg_popup
