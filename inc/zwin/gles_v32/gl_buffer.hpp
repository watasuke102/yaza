#pragma once

#include <wayland-server-core.h>
#include <zen-remote/server/gl-buffer.h>

#include <cassert>
#include <memory>

#include "common.hpp"
#include "util/data_pool.hpp"
#include "util/signal.hpp"
#include "util/weak_resource.hpp"

namespace yaza::zwin::gles_v32::gl_buffer {
class GlBuffer {
 public:
  DISABLE_MOVE_AND_COPY(GlBuffer);
  GlBuffer();
  ~GlBuffer();

  void commit();
  void sync(bool force_sync);

  void update_pending_data(uint32_t target, uint32_t usage, wl_resource* data);
  uint64_t remote_id() {
    assert(this->proxy_.has_value());
    return this->proxy_->get()->id();
  }

 private:
  struct {
    uint32_t                  target;
    uint32_t                  usage;
    util::WeakResource<void*> data;
  } pending_;
  struct {
    bool           data_damaged;
    uint32_t       target;
    uint32_t       usage;
    util::DataPool data;
    ssize_t        data_size;
  } current_;

  util::Listener<std::nullptr_t*> session_disconnected_listener_;
  std::optional<std::unique_ptr<zen::remote::server::IGlBuffer>> proxy_ =
      std::nullopt;
};

void create(wl_client* client, uint32_t id);
}  // namespace yaza::zwin::gles_v32::gl_buffer
