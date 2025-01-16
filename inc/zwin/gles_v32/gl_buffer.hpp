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
  explicit GlBuffer(wl_event_loop* loop);
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
    uint32_t                  target_;
    uint32_t                  usage_;
    util::WeakResource<void*> data_;
  } pending_;
  struct {
    bool           data_damaged_;
    uint32_t       target_;
    uint32_t       usage_;
    util::DataPool data_;
    ssize_t        data_size_;
  } current_;

  wl_event_loop*                  loop_;
  util::Listener<std::nullptr_t*> session_disconnected_listener_;
  std::optional<std::unique_ptr<zen::remote::server::IGlBuffer>> proxy_ =
      std::nullopt;
};

void create(wl_client* client, uint32_t id, wl_event_loop* loop);
}  // namespace yaza::zwin::gles_v32::gl_buffer
