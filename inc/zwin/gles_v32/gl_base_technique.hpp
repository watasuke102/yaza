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
#include <variant>

#include "common.hpp"
#include "util/signal.hpp"
#include "util/weak_resource.hpp"
#include "zwin/gles_v32/gl_program.hpp"
#include "zwin/gles_v32/gl_vertex_array.hpp"

// Do not include `rendering_unit.hpp` to prevent circular reference
namespace yaza::zwin::gles_v32::rendering_unit {
class RenderingUnit;
}

namespace yaza::zwin::gles_v32::gl_base_technique {
struct UniformVariable {
  UniformVariable(zwn_gl_base_technique_uniform_variable_type type,
      uint32_t location, const char* name, uint32_t col, uint32_t row,
      uint32_t count, bool transpose, void* value)
      : type_(type)
      , location_(location)
      , name_(name == nullptr ? "" : name)
      , col_(col)
      , row_(row)
      , count_(count)
      , transpose_(transpose)
      , value_(malloc(4UL * col * row), free) {
    std::memcpy(this->value_.get(), value, 4UL * col * row);
  };
  zwn_gl_base_technique_uniform_variable_type type_;
  uint32_t                                    location_;
  std::string                                 name_;
  uint32_t                                    col_;
  uint32_t                                    row_;
  uint32_t                                    count_;
  bool                                        transpose_;
  std::shared_ptr<void>                       value_;  // actually unique
  bool                                        newly_comitted_ = false;
};

// clang-format off
struct DrawArraysArgs;
template <template <typename> class T> struct DrawElementsArgs;
template <template <typename> class T>
using DrawApiArgs = std::variant<
  std::unique_ptr<DrawArraysArgs>,
  std::unique_ptr<DrawElementsArgs<T>>,
  std::nullopt_t
>;  // clang-format on

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
  void new_uniform_var(UniformVariable&& var);
  void request_draw_arrays(uint32_t mode, int32_t first, uint32_t count);
  void request_draw_elements(uint32_t mode, uint32_t count, uint32_t type,
      uint64_t offset, wl_resource* element_array_buffer);

 private:
  template <template <typename> class T>
  struct Buffer {
    bool                      program_changed_ = false;
    T<gl_program::GlProgram*> program_;

    bool                               vertex_array_changed_ = false;
    T<gl_vertex_array::GlVertexArray*> vertex_array_;

    bool           draw_api_args_changed_ = false;
    DrawApiArgs<T> draw_api_args_         = std::nullopt;

    std::list<UniformVariable> uniform_vars_;
  };
  Buffer<util::WeakResource> pending_;
  Buffer<std::optional>      current_;
  bool                       commited_ = false;

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
