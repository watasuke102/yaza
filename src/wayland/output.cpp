#include "wayland/output.hpp"

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>

#include <cstdint>

#include "common.hpp"

namespace yaza::wayland::output {
namespace {
void release(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
constexpr struct wl_output_interface kImpl = {
    .release = release,
};
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_output_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  LOG_DEBUG("output version: %u", version);
  wl_resource_set_implementation(resource, &kImpl, nullptr, nullptr);
  wl_output_send_geometry(resource, 0, 0, 0, 0, WL_OUTPUT_SUBPIXEL_UNKNOWN,
      "yaza", "PhantomOutput", WL_OUTPUT_TRANSFORM_NORMAL);
  wl_output_send_mode(resource, WL_OUTPUT_MODE_CURRENT, 960, 540, 0);
  if (version >= WL_OUTPUT_SCALE_SINCE_VERSION) {
    wl_output_send_scale(resource, 1);
  }
  if (version >= WL_OUTPUT_NAME_SINCE_VERSION) {
    wl_output_send_name(resource, "WL-1");
  }
  if (version >= WL_OUTPUT_DESCRIPTION_SINCE_VERSION) {
    wl_output_send_description(resource, "yaza phantom output");
  }
  if (version >= WL_OUTPUT_DONE_SINCE_VERSION) {
    wl_output_send_done(resource);
  }
}
}  // namespace yaza::wayland::output
