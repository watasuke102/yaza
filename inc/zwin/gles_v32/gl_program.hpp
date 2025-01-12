#pragma once

#include <wayland-server-core.h>
#include <zen-remote/server/gl-program.h>

#include <memory>
#include <vector>

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

  void attach_shader(std::weak_ptr<gl_shader::GlShader>&& shader);
  void request_link();

 private:
  struct {
    bool                                            damaged_     = false;
    bool                                            should_link_ = false;
    std::vector<std::weak_ptr<gl_shader::GlShader>> shaders_;
  } pending_;
  struct {
    bool                                            linked_      = false;
    bool                                            should_link_ = false;
    std::vector<std::weak_ptr<gl_shader::GlShader>> shaders_;
  } current_;
  wl_resource* resource_;

  util::Listener<std::nullptr_t*> session_disconnected_listener_;
  std::optional<std::unique_ptr<zen::remote::server::IGlProgram>> proxy_ =
      std::nullopt;
};

void create(wl_client* client, uint32_t id);
}  // namespace yaza::zwin::gles_v32::gl_program
