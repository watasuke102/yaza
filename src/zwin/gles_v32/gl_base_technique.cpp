#include "zwin/gles_v32/gl_base_technique.hpp"

#include <wayland-server-core.h>
#include <wayland-server.h>
#include <wayland-util.h>
#include <zen-remote/server/gl-base-technique.h>
#include <zen-remote/server/gl-buffer.h>
#include <zwin-gles-v32-protocol.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <optional>

#include "common.hpp"
#include "remote/remote.hpp"
#include "util/visitor_list.hpp"
#include "util/weakable_unique_ptr.hpp"
#include "zwin/gles_v32/gl_buffer.hpp"
#include "zwin/gles_v32/gl_program.hpp"
#include "zwin/gles_v32/gl_vertex_array.hpp"
#include "zwin/gles_v32/rendering_unit.hpp"

namespace yaza::zwin::gles_v32::gl_base_technique {
GlBaseTechnique::GlBaseTechnique(
    wl_resource* resource, rendering_unit::RenderingUnit* unit)
    : owner_(unit), resource_(resource) {
  this->session_disconnected_listener_.set_handler(
      [this](std::nullptr_t* /*data*/) {
        this->proxy_ = std::nullopt;
      });
  remote::g_remote->listen_session_disconnected(
      this->session_disconnected_listener_);

  this->owner_committed_listener_.set_handler([this](std::nullptr_t* /*data*/) {
    this->commit();
  });
  this->owner_->listen_commited(this->owner_committed_listener_);

  LOG_DEBUG("created: GlBaseTechnique");
}
GlBaseTechnique::~GlBaseTechnique() {
  LOG_DEBUG("destructor: GlBaseTechnique");
  this->owner_->set_technique(std::nullopt);
  wl_resource_set_user_data(this->resource_, nullptr);
  wl_resource_set_destructor(this->resource_, nullptr);
}

void GlBaseTechnique::commit() {
  if (!this->commited_) {
    this->owner_->set_technique(this);
    this->commited_ = true;
  }

  this->current_.program_changed_ = this->pending_.program_changed_;
  if (this->pending_.program_changed_) {
    this->current_.program_         = this->pending_.program_;
    this->pending_.program_changed_ = false;
  }
  if (auto* program = this->current_.program_.lock()) {
    program->commit();
  }

  this->current_.vertex_array_changed_ = this->pending_.vertex_array_changed_;
  if (this->pending_.vertex_array_changed_) {
    this->current_.vertex_array_         = this->pending_.vertex_array_;
    this->pending_.vertex_array_changed_ = false;
  }
  if (auto* vertex_array = this->current_.vertex_array_.lock()) {
    vertex_array->commit();
  }

  this->current_.draw_api_args_changed_ = this->pending_.draw_api_args_changed_;
  if (this->pending_.draw_api_args_changed_) {
    util::VisitorList(
        [this](std::unique_ptr<DrawArraysArgs>& args) {
          this->current_.draw_api_args_ = std::move(args);
        },
        [this](std::unique_ptr<DrawElementsArgs>& args) {
          if (auto* buf = args->element_array_buffer_.lock()) {
            buf->commit();
          }
          this->current_.draw_api_args_ = std::move(args);
        },
        [this](std::nullopt_t) {
          this->current_.draw_api_args_ = std::nullopt;
        })
        .visit(this->pending_.draw_api_args_);

    this->pending_.draw_api_args_         = std::nullopt;
    this->pending_.draw_api_args_changed_ = false;
  }

  for (auto& current : this->current_.uniform_vars_) {
    current.newly_comitted_ = false;
  }
  // This loop seems like a bad algorithm with O(n*m) time complexity,
  // but (basically) the number of Uniform Variables can be assumed
  // to be small enough, right?
  for (auto it = this->pending_.uniform_vars_.begin();
      it != this->pending_.uniform_vars_.end();) {
    auto& pending = *it;
    this->current_.uniform_vars_.remove_if(
        [&pending](UniformVariable& current) {
          if (!pending.name_.empty() && !current.name_.empty()) {
            return pending.name_ == current.name_;
          }
          if (pending.name_.empty() || current.name_.empty()) {
            return false;
          }
          return pending.location_ == current.location_;
        });
    pending.newly_comitted_ = true;
    this->current_.uniform_vars_.emplace_back(std::move(pending));
    it = this->pending_.uniform_vars_.erase(it);
  }
}

void GlBaseTechnique::sync(bool force_sync) {
  LOG_DEBUG("sync: GlBaseTechnique");
  if (!this->proxy_.has_value()) {
    this->proxy_ = zen::remote::server::CreateGlBaseTechnique(
        remote::g_remote->channel_nonnull(), this->owner_->remote_id());
  }
  const auto kShouldSync = [force_sync](bool changed) {
    return force_sync || changed;
  };

  if (auto* vertex_array = this->current_.vertex_array_.lock()) {
    vertex_array->sync(force_sync);
    if (kShouldSync(this->current_.vertex_array_changed_)) {
      this->proxy_->get()->BindVertexArray(vertex_array->remote_id());
    }
  }

  if (auto* program = this->current_.program_.lock()) {
    program->sync(force_sync);
    if (kShouldSync(this->current_.program_changed_)) {
      this->proxy_->get()->BindProgram(program->remote_id());
    }
  }

  for (auto& uniform_var : this->current_.uniform_vars_) {
    if (!kShouldSync(uniform_var.newly_comitted_)) {
      continue;
    }
    auto* value = uniform_var.value_.get();
    if (uniform_var.col_ == 1) {
      auto f = [this, &uniform_var](auto* value) {
        this->proxy_->get()->GlUniformVector(uniform_var.location_,
            uniform_var.name_, uniform_var.row_, uniform_var.count_, value);
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
      this->proxy_->get()->GlUniformMatrix(uniform_var.location_,
          uniform_var.name_, uniform_var.col_, uniform_var.row_,
          uniform_var.count_, uniform_var.transpose_,
          static_cast<float*>(value));
    }
  }

  util::VisitorList(
      [this, &kShouldSync](std::unique_ptr<DrawArraysArgs>& args) {
        if (kShouldSync(this->current_.draw_api_args_changed_)) {
          this->proxy_->get()->GlDrawArrays(
              args->mode_, args->first_, args->count_);
        }
      },
      [this, &kShouldSync, force_sync](
          std::unique_ptr<DrawElementsArgs>& args) {
        auto* element_array_buffer = args->element_array_buffer_.lock();
        if (!element_array_buffer) {
          return;
        }
        element_array_buffer->sync(force_sync);
        if (kShouldSync(this->current_.draw_api_args_changed_)) {
          this->proxy_->get()->GlDrawElements(args->mode_, args->count_,
              args->type_, args->offset_, element_array_buffer->remote_id());
        }
      })
      .visit(this->current_.draw_api_args_);
}

void GlBaseTechnique::request_bind_program(wl_resource* resource) {
  auto* tmp = static_cast<util::UniPtr<gl_program::GlProgram>*>(
      wl_resource_get_user_data(resource));
  this->pending_.program_         = (*tmp).weak();
  this->pending_.program_changed_ = true;
}
void GlBaseTechnique::request_bind_vertex_array(wl_resource* resource) {
  auto* tmp = static_cast<util::UniPtr<gl_vertex_array::GlVertexArray>*>(
      wl_resource_get_user_data(resource));
  this->pending_.vertex_array_         = (*tmp).weak();
  this->pending_.vertex_array_changed_ = true;
}
void GlBaseTechnique::new_uniform_var(UniformVariable&& var) {
  this->pending_.uniform_vars_.emplace_back(std::move(var));
}
void GlBaseTechnique::request_draw_arrays(
    uint32_t mode, int32_t first, uint32_t count) {
  this->pending_.draw_api_args_changed_ = true;
  this->pending_.draw_api_args_.emplace<std::unique_ptr<DrawArraysArgs>>(
      std::make_unique<DrawArraysArgs>(mode, first, count));
}
void GlBaseTechnique::request_draw_elements(uint32_t mode, uint32_t count,
    uint32_t type, uint64_t offset, wl_resource* element_array_buffer) {
  auto* tmp = static_cast<util::UniPtr<gl_buffer::GlBuffer>*>(
      wl_resource_get_user_data(element_array_buffer));
  this->pending_.draw_api_args_changed_ = true;
  this->pending_.draw_api_args_.emplace<std::unique_ptr<DrawElementsArgs>>(
      std::make_unique<DrawElementsArgs>(
          mode, count, type, offset, (*tmp).weak()));
}

namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void bind_program(
    wl_client* /*client*/, wl_resource* resource, wl_resource* program) {
  auto* self =
      static_cast<GlBaseTechnique*>(wl_resource_get_user_data(resource));
  if (self) {
    self->request_bind_program(program);
  }
}
void bind_vertex_array(
    wl_client* /*client*/, wl_resource* resource, wl_resource* vertex_array) {
  auto* self =
      static_cast<GlBaseTechnique*>(wl_resource_get_user_data(resource));
  if (self) {
    self->request_bind_vertex_array(vertex_array);
  }
}
void bind_texture(wl_client* client, wl_resource* /*resource*/,
    uint32_t /*binding*/, const char* /*name*/, wl_resource* /*texture*/,
    uint32_t /*target*/, wl_resource* /*sampler*/) {
  wl_client_post_implementation_error(client, "not yet implemented");
}
void uniform_vector(wl_client* /*client*/, wl_resource* resource,
    uint32_t location, const char* name, uint32_t type, uint32_t size,
    uint32_t count, wl_array* value) {
  auto* self =
      static_cast<GlBaseTechnique*>(wl_resource_get_user_data(resource));
  if (!self) {
    return;
  }

  if (size <= 0 || size > 4) {
    wl_resource_post_error(resource,
        ZWN_GL_BASE_TECHNIQUE_ERROR_UNIFORM_VARIABLE,
        "expect: 1 <= size <= 4, actual size: %d", size);
    return;
  }
  if (value->size < (4UL * size * count)) {
    wl_resource_post_error(resource,
        ZWN_GL_BASE_TECHNIQUE_ERROR_UNIFORM_VARIABLE,
        "expect: value.size >= %d, actual value.size: %lu", 4 * size * count,
        value->size);
    return;
  }

  auto type_enum =
      static_cast<zwn_gl_base_technique_uniform_variable_type>(type);
  self->new_uniform_var(UniformVariable(
      type_enum, location, name, 1, size, count, false, value->data));
}
void uniform_matrix(wl_client* /*client*/, wl_resource* resource,
    uint32_t location, const char* name, uint32_t col, uint32_t row,
    uint32_t count, uint32_t transpose, wl_array* value) {
  auto* self =
      static_cast<GlBaseTechnique*>(wl_resource_get_user_data(resource));
  if (!self) {
    return;
  }

  if (col <= 0 || col > 4) {
    wl_resource_post_error(resource,
        ZWN_GL_BASE_TECHNIQUE_ERROR_UNIFORM_VARIABLE,
        "expect: 1 <= col <= 4, actual col: %d", col);
    return;
  }
  if (row <= 0 || row > 4) {
    wl_resource_post_error(resource,
        ZWN_GL_BASE_TECHNIQUE_ERROR_UNIFORM_VARIABLE,
        "expect: 1 <= row <= 4, actual row: %d", row);
    return;
  }
  if (value->size < (4UL * col * row * count)) {
    wl_resource_post_error(resource,
        ZWN_GL_BASE_TECHNIQUE_ERROR_UNIFORM_VARIABLE,
        "expect: value.size >= %d, actual value.size: %lu",
        4 * col * row * count, value->size);
    return;
  }
  self->new_uniform_var(
      UniformVariable(ZWN_GL_BASE_TECHNIQUE_UNIFORM_VARIABLE_TYPE_FLOAT,
          location, name, col, row, count, transpose, value->data));
}
void draw_arrays(wl_client* /*client*/, wl_resource* resource, uint32_t mode,
    int32_t first, uint32_t count) {
  auto* self =
      static_cast<GlBaseTechnique*>(wl_resource_get_user_data(resource));
  if (self) {
    self->request_draw_arrays(mode, first, count);
  }
}
void draw_elements(wl_client* /*client*/, wl_resource* resource, uint32_t mode,
    uint32_t count, uint32_t type, wl_array* offset_array,
    wl_resource* element_array_buffer) {
  auto* self =
      static_cast<GlBaseTechnique*>(wl_resource_get_user_data(resource));
  if (!self) {
    return;
  }
  if (offset_array->size != sizeof(uint64_t)) {
    wl_resource_post_error(resource, ZWN_COMPOSITOR_ERROR_WL_ARRAY_SIZE,
        "offset size (%ld) does not equal uint64_t size (%ld)",
        offset_array->size, sizeof(uint64_t));
    return;
  }
  uint64_t offset = 0;
  std::memcpy(&offset, offset_array->data, offset_array->size);
  self->request_draw_elements(mode, count, type, offset, element_array_buffer);
}
const struct zwn_gl_base_technique_interface kImpl = {
    .destroy           = destroy,
    .bind_program      = bind_program,
    .bind_vertex_array = bind_vertex_array,
    .bind_texture      = bind_texture,
    .uniform_vector    = uniform_vector,
    .uniform_matrix    = uniform_matrix,
    .draw_arrays       = draw_arrays,
    .draw_elements     = draw_elements,
};

void destroy(wl_resource* resource) {
  auto* self =
      static_cast<GlBaseTechnique*>(wl_resource_get_user_data(resource));
  delete self;
}
}  // namespace
void create(
    wl_client* client, uint32_t id, rendering_unit::RenderingUnit* unit) {
  wl_resource* resource =
      wl_resource_create(client, &zwn_gl_base_technique_interface, 1, id);
  if (!resource) {
    wl_client_post_no_memory(client);
    return;
  }
  auto* self = new GlBaseTechnique(resource, unit);
  wl_resource_set_implementation(resource, &kImpl, self, destroy);
}
}  // namespace yaza::zwin::gles_v32::gl_base_technique
