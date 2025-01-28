#pragma once

#include <wayland-server-core.h>
#include <wayland-server.h>
#include <wayland-util.h>
#include <zen-remote/server/gl-shader.h>

#include <cassert>
#include <cstddef>
#include <optional>

#include "common.hpp"
#include "util/signal.hpp"
#include "zwin/shm/shm_buffer.hpp"

// Do not include `gl_program.hpp` to prevent circular reference
namespace yaza::zwin::gles_v32::gl_program {
class GlProgram;
}

namespace yaza::zwin::gles_v32::gl_shader {
class GlShader {
 public:
  DISABLE_MOVE_AND_COPY(GlShader);
  explicit GlShader(wl_resource* resource, zwin::shm_buffer::ShmBuffer* buffer,
      uint32_t type);
  ~GlShader();

  void     sync();
  uint64_t remote_id() {
    assert(this->proxy_.has_value());
    return this->proxy_->get()->id();
  }

 private:
  uint32_t                     type_;
  zwin::shm_buffer::ShmBuffer* buffer_;

  wl_resource*                    resource_;
  util::Listener<std::nullptr_t*> session_disconnected_listener_;
  std::optional<std::unique_ptr<zen::remote::server::IGlShader>> proxy_ =
      std::nullopt;
};

void create(wl_client* client, uint32_t id, wl_resource* buffer, uint32_t type);
}  // namespace yaza::zwin::gles_v32::gl_shader
