#include "zwin/gles_v32/gl_vertex_array.hpp"

#include <wayland-server-core.h>
#include <wayland-server.h>
#include <wayland-util.h>
#include <zen-remote/server/gl-vertex-array.h>
#include <zwin-gles-v32-protocol.h>

#include <cstdint>
#include <cstring>
#include <memory>

#include "common.hpp"
#include "server.hpp"
#include "util/weakable_unique_ptr.hpp"
#include "zwin/gles_v32/gl_buffer.hpp"

namespace yaza::zwin::gles_v32::gl_vertex_array {
struct VertexAttribute {
  int32_t                            size_;
  uint32_t                           type_;
  int32_t                            stride_;
  uint64_t                           offset_;
  bool                               normalized_;
  bool                               gl_buffer_changed_;
  util::WeakPtr<gl_buffer::GlBuffer> gl_buffer_;

  bool enability_changed_;
  bool enabled_;
  void set_enability(bool enabled) {
    this->enability_changed_ = true;
    this->enabled_           = enabled;
  }
};

GlVertexArray::GlVertexArray() {
  this->session_disconnected_listener_.set_handler(
      [this](std::nullptr_t* /*data*/) {
        this->proxy_ = std::nullopt;
      });
  server::get().remote->listen_session_disconnected(
      this->session_disconnected_listener_);
  LOG_DEBUG("created: GlVertexArray");
}
GlVertexArray::~GlVertexArray() {
  LOG_DEBUG("destructor: GlVertexArray");
}

void GlVertexArray::commit() {
  this->current_.attribute_map_.clear();

  for (auto [index, attrib] : this->pending_.attribute_map_) {
    this->current_.attribute_map_.emplace(index, attrib);
    attrib.enability_changed_ = false;
    attrib.gl_buffer_changed_ = false;

    if (auto* buffer = attrib.gl_buffer_.lock()) {
      buffer->commit();
    }
  }
}

void GlVertexArray::sync(bool force_sync) {
  if (!this->proxy_.has_value()) {
    this->proxy_ = zen::remote::server::CreateGlVertexArray(
        server::get().remote->channel_nonnull());
  }
  for (auto& [index, attrib] : this->current_.attribute_map_) {
    auto* buffer = attrib.gl_buffer_.lock();
    if (!buffer) {
      continue;
    }
    buffer->sync(force_sync);

    if (force_sync || attrib.enability_changed_) {
      if (attrib.enabled_) {
        this->proxy_->get()->GlEnableVertexAttribArray(index);
      } else {
        this->proxy_->get()->GlDisableVertexAttribArray(index);
      }
    }
    if (force_sync || attrib.gl_buffer_changed_) {
      this->proxy_->get()->GlVertexAttribPointer(index, attrib.size_,
          attrib.type_, attrib.normalized_, attrib.stride_, attrib.offset_,
          buffer->remote_id());
    }
  }
}

VertexAttribute& GlVertexArray::get_pending_attribute(uint32_t index) {
  return this->pending_.attribute_map_[index];
}

namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void enable_vertex_attrib_array(
    wl_client* /*client*/, wl_resource* resource, uint32_t index) {
  auto* self = static_cast<util::UniPtr<GlVertexArray>*>(
      wl_resource_get_user_data(resource));
  self->get()->get_pending_attribute(index).set_enability(true);
}
void disable_vertex_attrib_array(
    wl_client* /*client*/, wl_resource* resource, uint32_t index) {
  auto* self = static_cast<util::UniPtr<GlVertexArray>*>(
      wl_resource_get_user_data(resource));
  self->get()->get_pending_attribute(index).set_enability(false);
}
void vertex_attrib_pointer(wl_client* /*client*/, wl_resource* resource,
    uint32_t index, int32_t size, uint32_t type, uint32_t normalized,
    int32_t stride, wl_array* offset, wl_resource* gl_buffer) {
  if (offset->size != sizeof(uint64_t)) {
    wl_resource_post_error(resource, ZWN_COMPOSITOR_ERROR_WL_ARRAY_SIZE,
        "offset size (%ld) does not equal uint64_t size (%ld)", offset->size,
        sizeof(uint64_t));
    return;
  }
  auto* self = static_cast<util::UniPtr<GlVertexArray>*>(
      wl_resource_get_user_data(resource));
  auto* buffer = static_cast<util::UniPtr<gl_buffer::GlBuffer>*>(
      wl_resource_get_user_data(gl_buffer));

  auto& attrib = self->get()->get_pending_attribute(index);
  std::memcpy(&attrib.offset_, offset->data, offset->size);
  attrib.size_              = size;
  attrib.type_              = type;
  attrib.normalized_        = normalized;
  attrib.stride_            = stride;
  attrib.gl_buffer_changed_ = true;
  attrib.gl_buffer_         = buffer->weak();
}
const struct zwn_gl_vertex_array_interface kImpl = {
    .destroy                     = destroy,
    .enable_vertex_attrib_array  = enable_vertex_attrib_array,
    .disable_vertex_attrib_array = disable_vertex_attrib_array,
    .vertex_attrib_pointer       = vertex_attrib_pointer,
};

void destroy(wl_resource* resource) {
  auto* self = static_cast<util::UniPtr<GlVertexArray>*>(
      wl_resource_get_user_data(resource));
  delete self;
}
}  // namespace
void create(wl_client* client, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &zwn_gl_vertex_array_interface, 1, id);
  if (!resource) {
    wl_client_post_no_memory(client);
    return;
  }
  auto* self = new util::UniPtr<GlVertexArray>();
  wl_resource_set_implementation(resource, &kImpl, self, destroy);
}
}  // namespace yaza::zwin::gles_v32::gl_vertex_array
