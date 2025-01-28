#include "zwin/gles_v32/base_technique/draw_api_args.hpp"

#include <wayland-server-core.h>
#include <wayland-server.h>
#include <zen-remote/server/gl-base-technique.h>
#include <zwin-gles-v32-protocol.h>

#include <cassert>
#include <cstring>
#include <optional>

#include "common.hpp"
#include "util/visitor_list.hpp"

namespace yaza::zwin::gles_v32::gl_base_technique {
DrawArraysArgs::DrawArraysArgs(uint32_t mode, int32_t first, uint32_t count)
    : mode(mode), first(first), count(count) {
  LOG_DEBUG("constructor: DrawArraysArgs");
}
DrawArraysArgs::~DrawArraysArgs() {
  LOG_DEBUG(" destructor: DrawArraysArgs");
}

DrawElementsArgs::DrawElementsArgs(uint32_t mode, uint32_t count, uint32_t type,
    uint64_t offset, util::WeakPtr<gl_buffer::GlBuffer>&& element_array_buffer)
    : mode(mode), count(count), type(type), offset(offset) {
  this->element_array_buffer = std::move(element_array_buffer);
}
DrawElementsArgs ::~DrawElementsArgs() {
  LOG_DEBUG(" destructor: DrawElementsArgs");
}

DrawApiArgs::DrawApiArgs() : data_(std::nullopt) {
}
void DrawApiArgs::commit(DrawApiArgs& pending, DrawApiArgs& current) {
  current.changed_ = pending.changed_;
  if (pending.changed_) {
    util::VisitorList(
        [&current](std::unique_ptr<DrawArraysArgs>& args) {
          current.data_ = std::move(args);
        },
        [&current](std::unique_ptr<DrawElementsArgs>& args) {
          if (auto* buf = args->element_array_buffer.lock()) {
            buf->commit();
          }
          current.data_ = std::move(args);
        },
        [&current](std::nullopt_t) {
          current.data_ = std::nullopt;
        })
        .visit(pending.data_);
    pending.data_    = std::nullopt;
    pending.changed_ = false;
  }
}
void DrawApiArgs::sync(
    std::unique_ptr<zen::remote::server::IGlBaseTechnique>& proxy,
    bool                                                    force_sync) {
  const bool should_sync = force_sync || this->changed_;
  util::VisitorList(
      [&proxy, should_sync](std::unique_ptr<DrawArraysArgs>& args) {
        if (should_sync) {
          proxy->GlDrawArrays(args->mode, args->first, args->count);
        }
      },
      [&proxy, should_sync, force_sync](
          std::unique_ptr<DrawElementsArgs>& args) {
        auto* element_array_buffer = args->element_array_buffer.lock();
        if (!element_array_buffer) {
          return;
        }
        element_array_buffer->sync(force_sync);
        if (should_sync) {
          proxy->GlDrawElements(args->mode, args->count, args->type,
              args->offset, element_array_buffer->remote_id());
        }
      })
      .visit(this->data_);
}

void DrawApiArgs::set_arrays_args(
    uint32_t mode, int32_t first, uint32_t count) {
  this->changed_ = true;
  this->data_.emplace<std::unique_ptr<DrawArraysArgs>>(
      std::make_unique<DrawArraysArgs>(mode, first, count));
}
void DrawApiArgs::set_elements_args(uint32_t mode, uint32_t count,
    uint32_t type, uint64_t offset,
    util::WeakPtr<gl_buffer::GlBuffer>&& element_array_buffer) {
  this->changed_ = true;
  this->data_.emplace<std::unique_ptr<DrawElementsArgs>>(
      std::make_unique<DrawElementsArgs>(
          mode, count, type, offset, std::move(element_array_buffer)));
}
}  // namespace yaza::zwin::gles_v32::gl_base_technique
