#pragma once

#include <wayland-server-core.h>
#include <zen-remote/server/gl-program.h>

#include <list>
#include <memory>

#include "common.hpp"
#include "zwin/gles_v32/gl_shader.hpp"

namespace yaza::zwin::gles_v32::gl_program {
class GlProgram {
 public:
  DISABLE_MOVE_AND_COPY(GlProgram);
  explicit GlProgram(wl_resource* resource);
  ~GlProgram();

  void commit();
  void sync(bool force_sync);

  void request_link();
  void attach_shader(gl_shader::GlShader* shader);
  void remove_shader(gl_shader::GlShader* shader);

 private:
  struct {
    bool                            damaged_     = false;
    bool                            should_link_ = false;
    std::list<gl_shader::GlShader*> shaders_;
  } pending_;
  struct {
    bool                            linked_      = false;
    bool                            should_link_ = false;
    std::list<gl_shader::GlShader*> shaders_;
  } current_;
  wl_resource* resource_;

  util::Listener<std::nullptr_t*> session_disconnected_listener_;
  std::optional<std::unique_ptr<zen::remote::server::IGlProgram>> proxy_ =
      std::nullopt;
};

void create(wl_client* client, uint32_t id);
}  // namespace yaza::zwin::gles_v32::gl_program
