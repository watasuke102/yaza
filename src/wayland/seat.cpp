#include "wayland/seat.hpp"

#include <GLES3/gl32.h>
#include <linux/input-event-codes.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-server.h>
#include <wayland-util.h>

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>

#include "server.hpp"
#include "wayland/keyboard.hpp"
#include "wayland/pointer.hpp"

namespace yaza::wayland::seat {
ClientSeat::ClientSeat(wl_resource* resource) : resource_(resource) {
  wl_list_init(&this->pointers_);
  wl_list_init(&this->keyboards_);
  wl_list_init(&this->data_devices_);
  wl_list_init(&this->rays_);
}
ClientSeat::~ClientSeat() {
  wl_list_remove(&this->pointers_);
  wl_list_remove(&this->keyboards_);
  wl_list_remove(&this->data_devices_);
  wl_list_remove(&this->rays_);
}

void ClientSeat::add_pointer(wl_resource* resource) {
  wl_list_insert(&this->pointers_, wl_resource_get_link(resource));
}
void ClientSeat::add_keyboard(wl_resource* resource) {
  wl_list_insert(&this->keyboards_, wl_resource_get_link(resource));
}
void ClientSeat::add_data_device(wl_resource* resource) {
  wl_list_insert(&this->data_devices_, wl_resource_get_link(resource));
}
void ClientSeat::add_ray(wl_resource* resource) {
  wl_list_insert(&this->rays_, wl_resource_get_link(resource));
}

void ClientSeat::pointer_foreach(
    const std::function<void(wl_resource*)>& handler) const {
  wl_resource* resource = nullptr;
  wl_resource_for_each(resource, &this->pointers_) {
    handler(resource);
  }
}
void ClientSeat::keyboard_foreach(
    const std::function<void(wl_resource*)>& handler) const {
  wl_resource* resource = nullptr;
  wl_resource_for_each(resource, &this->keyboards_) {
    handler(resource);
  }
}
void ClientSeat::data_device_foreach(
    const std::function<void(wl_resource*)>& handler) const {
  wl_resource* resource = nullptr;
  wl_resource_for_each(resource, &this->data_devices_) {
    handler(resource);
  }
}
void ClientSeat::ray_foreach(
    const std::function<void(wl_resource*)>& handler) const {
  wl_resource* resource = nullptr;
  wl_resource_for_each(resource, &this->rays_) {
    handler(resource);
  }
}

namespace {
void get_pointer(wl_client* client, wl_resource* /*resource*/, uint32_t id) {
  pointer::create(client, id);
}
void get_keyboard(wl_client* client, wl_resource* /*resource*/, uint32_t id) {
  keyboard::create(client, id);
}
void get_touch(wl_client* client, wl_resource* /*resource*/, uint32_t /*id*/) {
  wl_client_post_implementation_error(
      client, "touch device is not implemented");
}
void release(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
constexpr struct wl_seat_interface kImpl = {
    .get_pointer  = get_pointer,
    .get_keyboard = get_keyboard,
    .get_touch    = get_touch,
    .release      = release,
};

void destroy(wl_resource* resource) {
  server::get().seat->client_seats.erase(wl_resource_get_client(resource));
  delete get(resource);
}
}  // namespace

void bind(wl_client* client, void* /*data*/, uint32_t version, uint32_t id) {
  wl_resource* resource = wl_resource_create(
      client, &wl_seat_interface, static_cast<int>(version), id);
  if (resource == nullptr) {
    wl_client_post_no_memory(client);
    return;
  }
  auto* self = new ClientSeat(resource);
  server::get().seat->client_seats.emplace(client, self);
  wl_resource_set_implementation(resource, &kImpl, self, destroy);
  wl_seat_send_capabilities(
      resource, WL_SEAT_CAPABILITY_POINTER | WL_SEAT_CAPABILITY_KEYBOARD);
  LOG_DEBUG("created: ClientSeat for client %p", (void*)client);
}
ClientSeat* get(wl_resource* resource) {
  assert(wl_resource_instance_of(resource, &wl_seat_interface, &kImpl));
  return static_cast<ClientSeat*>(wl_resource_get_user_data(resource));
}
}  // namespace yaza::wayland::seat
