#include "zwin/gles_v32/base_technique/uniform_variables.hpp"

#include <wayland-server-core.h>
#include <wayland-server.h>
#include <zen-remote/server/gl-base-technique.h>
#include <zwin-gles-v32-protocol.h>

#include <cassert>
#include <cstring>
#include <memory>

namespace yaza::zwin::gles_v32::gl_base_technique {
UniformVariable::UniformVariable(
    zwn_gl_base_technique_uniform_variable_type type, uint32_t location,
    const char* name, uint32_t col, uint32_t row, uint32_t count,
    bool transpose, void* value)
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

void UniformVariableList::emplace(
    zwn_gl_base_technique_uniform_variable_type type, uint32_t location,
    const char* name, uint32_t col, uint32_t row, uint32_t count,
    bool transpose, void* value) {
  this->list_.emplace_back(
      type, location, name, col, row, count, transpose, value);
}

void UniformVariableList::commit(
    UniformVariableList& pending, UniformVariableList& current) {
  for (auto& current : current.list_) {
    current.newly_comitted_ = false;
  }
  // This loop seems like a bad algorithm with O(n*m) time complexity,
  // but (basically) the number of Uniform Variables can be assumed
  // to be small enough, right?
  for (auto it = pending.list_.begin(); it != pending.list_.end();) {
    auto& pending_var = *it;
    current.list_.remove_if([&pending_var](UniformVariable& current_var) {
      if (!pending_var.name_.empty() && !current_var.name_.empty()) {
        return pending_var.name_ == current_var.name_;
      }
      if (pending_var.name_.empty() || current_var.name_.empty()) {
        return false;
      }
      return pending_var.location_ == current_var.location_;
    });
    pending_var.newly_comitted_ = true;
    current.list_.emplace_back(std::move(pending_var));
    it = pending.list_.erase(it);
  }
}
void UniformVariableList::sync(
    std::unique_ptr<zen::remote::server::IGlBaseTechnique>& proxy,
    bool                                                    force_sync) {
  for (auto& uniform_var : this->list_) {
    if (!force_sync && !uniform_var.newly_comitted_) {
      continue;
    }
    auto* value = uniform_var.value_.get();
    if (uniform_var.col_ == 1) {
      auto f = [&proxy, &uniform_var](auto* value) {
        proxy->GlUniformVector(uniform_var.location_, uniform_var.name_,
            uniform_var.row_, uniform_var.count_, value);
      };
      switch (uniform_var.type_) {
        case ZWN_GL_BASE_TECHNIQUE_UNIFORM_VARIABLE_TYPE_INT:
          f(static_cast<int32_t*>(value));
          break;
        case ZWN_GL_BASE_TECHNIQUE_UNIFORM_VARIABLE_TYPE_UINT:
          f(static_cast<uint32_t*>(value));
          break;
        case ZWN_GL_BASE_TECHNIQUE_UNIFORM_VARIABLE_TYPE_FLOAT:
          f(static_cast<float*>(value));
          break;
        default:
          // unreachable!()
          break;
      }
    } else {
      proxy->GlUniformMatrix(uniform_var.location_, uniform_var.name_,
          uniform_var.col_, uniform_var.row_, uniform_var.count_,
          uniform_var.transpose_, static_cast<float*>(value));
    }
  }
}
}  // namespace yaza::zwin::gles_v32::gl_base_technique
