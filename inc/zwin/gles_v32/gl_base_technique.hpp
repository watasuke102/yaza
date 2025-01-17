#pragma once

#include <wayland-server-core.h>
#include <wayland-server.h>
#include <zen-remote/server/gl-base-technique.h>
#include <zwin-gles-v32-protocol.h>

#include <cassert>
#include <cstddef>
#include <cstring>
#include <memory>
#include <optional>

#include "common.hpp"
#include "util/signal.hpp"
#include "util/weakable_unique_ptr.hpp"
#include "zwin/gles_v32/base_technique/draw_api_args.hpp"
#include "zwin/gles_v32/base_technique/texture_binding.hpp"
#include "zwin/gles_v32/base_technique/uniform_variables.hpp"
#include "zwin/gles_v32/gl_program.hpp"
#include "zwin/gles_v32/gl_sampler.hpp"
#include "zwin/gles_v32/gl_texture.hpp"
#include "zwin/gles_v32/gl_vertex_array.hpp"

// Do not include `rendering_unit.hpp` to prevent circular reference
namespace yaza::zwin::gles_v32::rendering_unit {
class RenderingUnit;
}

namespace yaza::zwin::gles_v32::gl_base_technique {
class GlBaseTechnique {
 public:
  DISABLE_MOVE_AND_COPY(GlBaseTechnique);
  explicit GlBaseTechnique(
      wl_resource* resource, rendering_unit::RenderingUnit* unit);
  ~GlBaseTechnique();

  void     commit();
  void     sync(bool force_sync);
  uint64_t remote_id() {
    assert(this->proxy_.has_value());
    return this->proxy_->get()->id();
  }

  void request_bind_program(wl_resource* resource);
  void request_bind_vertex_array(wl_resource* resource);
  void request_bind_texture(uint32_t binding, const char* name, uint32_t target,
      util::WeakPtr<gl_texture::GlTexture>&& texture,
      util::WeakPtr<gl_sampler::GlSampler>&& sampler);
  void new_uniform_var(zwn_gl_base_technique_uniform_variable_type type,
      uint32_t location, const char* name, uint32_t col, uint32_t row,
      uint32_t count, bool transpose, void* value);
  void request_draw_arrays(uint32_t mode, int32_t first, uint32_t count);
  void request_draw_elements(uint32_t mode, uint32_t count, uint32_t type,
      uint64_t offset, wl_resource* element_array_buffer);

 private:
  struct {
    bool                                 program_changed_ = false;
    util::WeakPtr<gl_program::GlProgram> program_;

    bool                                          vertex_array_changed_ = false;
    util::WeakPtr<gl_vertex_array::GlVertexArray> vertex_array_;

    TextureBindingList  texture_bindings_;
    DrawApiArgs         draw_api_args_;
    UniformVariableList uniform_vars_;
  } pending_, current_;
  bool commited_ = false;

  // nonnull; Note that GlBaseTechnique is destroyed
  // when RenderingUnit is going to be destroyed
  rendering_unit::RenderingUnit*  owner_;
  util::Listener<std::nullptr_t*> owner_committed_listener_;

  wl_resource*                    resource_;
  util::Listener<std::nullptr_t*> session_disconnected_listener_;
  std::optional<std::unique_ptr<zen::remote::server::IGlBaseTechnique>> proxy_ =
      std::nullopt;
};

void create(
    wl_client* client, uint32_t id, rendering_unit::RenderingUnit* unit);
}  // namespace yaza::zwin::gles_v32::gl_base_technique
