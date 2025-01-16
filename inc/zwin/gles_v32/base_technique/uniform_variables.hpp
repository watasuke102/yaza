#pragma once

#include <wayland-server-core.h>
#include <wayland-server.h>
#include <zen-remote/server/gl-base-technique.h>
#include <zwin-gles-v32-protocol.h>

#include <cassert>
#include <cstring>

#include "common.hpp"

namespace yaza::zwin::gles_v32::gl_base_technique {
struct UniformVariable {
  UniformVariable(zwn_gl_base_technique_uniform_variable_type type,
      uint32_t location, const char* name, uint32_t col, uint32_t row,
      uint32_t count, bool transpose, void* value);

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

class UniformVariableList {
 public:
  DISABLE_MOVE_AND_COPY(UniformVariableList);
  UniformVariableList()  = default;
  ~UniformVariableList() = default;

  static void commit(
      UniformVariableList& pending, UniformVariableList& current);
  void sync(std::unique_ptr<zen::remote::server::IGlBaseTechnique>& proxy,
      bool                                                          force_sync);

  void emplace(zwn_gl_base_technique_uniform_variable_type type,
      uint32_t location, const char* name, uint32_t col, uint32_t row,
      uint32_t count, bool transpose, void* value);

 private:
  std::list<UniformVariable> list_;
};
}  // namespace yaza::zwin::gles_v32::gl_base_technique
