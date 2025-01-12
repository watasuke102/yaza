#pragma once

#include <wayland-server-core.h>
#include <wayland-server.h>
#include <wayland-util.h>
#include <zen-remote/server/virtual-object.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>

#include "common.hpp"
#include "remote/session.hpp"
#include "util/signal.hpp"

namespace yaza::zwin::virtual_object {
class VirtualObject {
 public:
  DISABLE_MOVE_AND_COPY(VirtualObject);
  VirtualObject();
  ~VirtualObject();

  void     commit();
  void     sync(bool force_sync);
  uint64_t remote_id() {
    assert(this->proxy_.has_value());
    return this->proxy_->get()->id();
  }

  void queue_frame_callback(wl_resource* callback_resource) const;
  void send_frame_done();

 private:
  struct {
    util::Signal<std::nullptr_t*> begin_commit_;
    util::Signal<std::nullptr_t*> destroy_;
  } events_;

  struct {
    wl_list frame_callback_list_;
  } pending_, current_;
  bool committed_ = false;

  util::Listener<remote::Session*> session_established_listener_;
  util::Listener<std::nullptr_t*>  session_disconnected_listener_;
  util::Listener<std::nullptr_t*>  session_frame_listener_;
  std::optional<std::unique_ptr<zen::remote::server::IVirtualObject>> proxy_ =
      std::nullopt;
};
void create(wl_client* client, uint32_t id);
}  // namespace yaza::zwin::virtual_object
