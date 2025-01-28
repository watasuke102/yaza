#pragma once

#include <wayland-server-core.h>
#include <zen-remote/server/gl-program.h>

#include <list>
#include <memory>

#include "common.hpp"
#include "util/weakable_unique_ptr.hpp"
#include "zwin/gles_v32/gl_shader.hpp"

namespace yaza::zwin::gles_v32::gl_program {
class GlProgram {
 public:
  DISABLE_MOVE_AND_COPY(GlProgram);
  explicit GlProgram(wl_resource* resource);
  ~GlProgram();

  void     commit();
  void     sync(bool force_sync);
  uint64_t remote_id() {
    assert(this->proxy_.has_value());
    return this->proxy_->get()->id();
  }

  void request_link();
  void attach_shader(util::WeakPtr<gl_shader::GlShader>&& shader);

 private:
  struct {
    bool                                          damaged     = false;
    bool                                          should_link = false;
    std::list<util::WeakPtr<gl_shader::GlShader>> shaders;
  } pending_;
  struct {
    bool                                          linked      = false;
    bool                                          should_link = false;
    std::list<util::WeakPtr<gl_shader::GlShader>> shaders;
  } current_;
  wl_resource* resource_;

  util::Listener<std::nullptr_t*> session_disconnected_listener_;
  std::optional<std::unique_ptr<zen::remote::server::IGlProgram>> proxy_ =
      std::nullopt;
};

void create(wl_client* client, uint32_t id);
}  // namespace yaza::zwin::gles_v32::gl_program
