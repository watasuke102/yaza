#pragma once

#include <wayland-server-core.h>
#include <zen-remote/server/gl-vertex-array.h>

#include <cassert>
#include <cstddef>
#include <memory>
#include <unordered_map>

#include "common.hpp"
#include "util/signal.hpp"

namespace yaza::zwin::gles_v32::gl_vertex_array {
struct VertexAttribute;

class GlVertexArray {
 public:
  DISABLE_MOVE_AND_COPY(GlVertexArray);
  GlVertexArray();
  ~GlVertexArray();

  void     commit();
  void     sync(bool force_sync);
  uint64_t remote_id() {
    assert(this->proxy_.has_value());
    return this->proxy_->get()->id();
  }

  VertexAttribute& get_pending_attribute(uint32_t index);

 private:
  struct {
    std::unordered_map<uint32_t, VertexAttribute> attribute_map;
  } pending_, current_;

  util::Listener<std::nullptr_t*> session_disconnected_listener_;
  std::optional<std::unique_ptr<zen::remote::server::IGlVertexArray>> proxy_ =
      std::nullopt;
};

void create(wl_client* client, uint32_t id);
}  // namespace yaza::zwin::gles_v32::gl_vertex_array
