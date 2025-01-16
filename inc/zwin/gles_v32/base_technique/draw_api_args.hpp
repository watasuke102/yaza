#pragma once

#include <wayland-server-core.h>
#include <wayland-server.h>
#include <zen-remote/server/gl-base-technique.h>
#include <zwin-gles-v32-protocol.h>

#include <cassert>
#include <cstring>
#include <memory>
#include <optional>
#include <variant>

#include "common.hpp"
#include "util/weakable_unique_ptr.hpp"
#include "zwin/gles_v32/gl_buffer.hpp"

namespace yaza::zwin::gles_v32::gl_base_technique {
struct DrawArraysArgs {
  DrawArraysArgs(uint32_t mode, int32_t first, uint32_t count);
  DrawArraysArgs(const DrawArraysArgs&)            = default;
  DrawArraysArgs(DrawArraysArgs&&)                 = default;
  DrawArraysArgs& operator=(const DrawArraysArgs&) = default;
  DrawArraysArgs& operator=(DrawArraysArgs&&)      = default;
  ~DrawArraysArgs();

  uint32_t mode_;
  int32_t  first_;
  uint32_t count_;
};

struct DrawElementsArgs {
  DrawElementsArgs(uint32_t mode, uint32_t count, uint32_t type,
      uint64_t                             offset,
      util::WeakPtr<gl_buffer::GlBuffer>&& element_array_buffer);
  DrawElementsArgs(const DrawElementsArgs&)            = default;
  DrawElementsArgs(DrawElementsArgs&&)                 = default;
  DrawElementsArgs& operator=(const DrawElementsArgs&) = default;
  DrawElementsArgs& operator=(DrawElementsArgs&&)      = default;
  ~DrawElementsArgs();

  uint32_t                           mode_;
  uint32_t                           count_;
  uint32_t                           type_;
  uint64_t                           offset_;
  util::WeakPtr<gl_buffer::GlBuffer> element_array_buffer_;
};

class DrawApiArgs {
 public:
  DISABLE_MOVE_AND_COPY(DrawApiArgs);
  DrawApiArgs();
  ~DrawApiArgs() = default;

  static void commit(DrawApiArgs& pending, DrawApiArgs& current);
  void sync(std::unique_ptr<zen::remote::server::IGlBaseTechnique>& proxy,
      bool                                                          force_sync);

  void set_arrays_args(uint32_t mode, int32_t first, uint32_t count);
  void set_elements_args(uint32_t mode, uint32_t count, uint32_t type,
      uint64_t                             offset,
      util::WeakPtr<gl_buffer::GlBuffer>&& element_array_buffer);

 private:
  // clang-format off
  std::variant<
    std::unique_ptr<DrawArraysArgs>,
    std::unique_ptr<DrawElementsArgs>,
    std::nullopt_t
  > data_;  // clang-format on
  bool changed_ = false;
};
}  // namespace yaza::zwin::gles_v32::gl_base_technique
