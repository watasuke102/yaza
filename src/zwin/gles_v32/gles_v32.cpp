#include "zwin/gles_v32/gles_v32.hpp"

#include <wayland-server-core.h>
#include <wayland-server.h>
#include <zwin-gles-v32-protocol.h>

#include <cstdint>

#include "server.hpp"
#include "zwin/gles_v32/gl_buffer.hpp"
#include "zwin/gles_v32/gl_program.hpp"
#include "zwin/gles_v32/gl_shader.hpp"

namespace yaza::zwin::gles_v32 {
namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void create_rendering_unit(wl_client* /*client*/, wl_resource* /*resource*/,
    uint32_t /*id*/, wl_resource* /*virtual_object*/) {
  // TODO!
}
void create_gl_buffer(wl_client* client, wl_resource* resource, uint32_t id) {
  auto* server =
      static_cast<yaza::Server*>(wl_resource_get_user_data(resource));
  gl_buffer::create(client, id, server->loop());
}
void create_gl_shader(wl_client* client, wl_resource* /*resource*/, uint32_t id,
    wl_resource* buffer, uint32_t type) {
  gl_shader::create(client, id, buffer, type);
}
void create_gl_program(
    wl_client* client, wl_resource* /*resource*/, uint32_t id) {
  gl_program::create(client, id);
}
void create_gl_texture(
    wl_client* /*client*/, wl_resource* /*resource*/, uint32_t /*id*/) {
  // TODO
}
void create_gl_sampler(
    wl_client* /*client*/, wl_resource* /*resource*/, uint32_t /*id*/) {
  // TODO
}
void create_gl_vertex_array(
    wl_client* /*client*/, wl_resource* /*resource*/, uint32_t /*id*/) {
  // TODO!
}
void create_gl_base_technique(wl_client* /*client*/, wl_resource* /*resource*/,
    uint32_t /*id*/, wl_resource* /*unit*/) {
  // TODO!
}
const struct zwn_gles_v32_interface kImpl = {
    .destroy                  = destroy,
    .create_rendering_unit    = create_rendering_unit,
    .create_gl_buffer         = create_gl_buffer,
    .create_gl_shader         = create_gl_shader,
    .create_gl_program        = create_gl_program,
    .create_gl_texture        = create_gl_texture,
    .create_gl_sampler        = create_gl_sampler,
    .create_gl_vertex_array   = create_gl_vertex_array,
    .create_gl_base_technique = create_gl_base_technique,
};
}  // namespace

void bind(wl_client* client, void* data, uint32_t version, uint32_t id) {
  auto* server = static_cast<yaza::Server*>(data);

  wl_resource* resource = wl_resource_create(
      client, &zwn_gles_v32_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  wl_resource_set_implementation(resource, &kImpl, server, nullptr);
}
}  // namespace yaza::zwin::gles_v32
