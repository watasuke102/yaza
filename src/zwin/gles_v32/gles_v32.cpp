#include "zwin/gles_v32/gles_v32.hpp"

#include <wayland-server-core.h>
#include <wayland-server.h>
#include <zwin-gles-v32-protocol.h>

#include <cstdint>

#include "server.hpp"
#include "zwin/gles_v32/gl_base_technique.hpp"
#include "zwin/gles_v32/gl_buffer.hpp"
#include "zwin/gles_v32/gl_program.hpp"
#include "zwin/gles_v32/gl_sampler.hpp"
#include "zwin/gles_v32/gl_shader.hpp"
#include "zwin/gles_v32/gl_texture.hpp"
#include "zwin/gles_v32/gl_vertex_array.hpp"
#include "zwin/gles_v32/rendering_unit.hpp"
#include "zwin/virtual_object.hpp"

namespace yaza::zwin::gles_v32 {
namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void create_rendering_unit(wl_client* client, wl_resource* /*resource*/,
    uint32_t id, wl_resource* virtual_object_resource) {
  auto* virtual_object = static_cast<virtual_object::VirtualObject*>(
      wl_resource_get_user_data(virtual_object_resource));
  rendering_unit::create(client, id, virtual_object);
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
void create_gl_texture(wl_client* client, wl_resource* resource, uint32_t id) {
  auto* server =
      static_cast<yaza::Server*>(wl_resource_get_user_data(resource));
  gl_texture::create(client, id, server->loop());
}
void create_gl_sampler(
    wl_client* client, wl_resource* /*resource*/, uint32_t id) {
  gl_sampler::create(client, id);
}
void create_gl_vertex_array(
    wl_client* client, wl_resource* /*resource*/, uint32_t id) {
  gl_vertex_array::create(client, id);
}
void create_gl_base_technique(wl_client* client, wl_resource* /*resource*/,
    uint32_t id, wl_resource* unit_resource) {
  auto* unit = static_cast<rendering_unit::RenderingUnit*>(
      wl_resource_get_user_data(unit_resource));
  if (!unit) {
    wl_client_post_implementation_error(client, "RenderingUnit is null");
    return;
  }
  gl_base_technique::create(client, id, unit);
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
