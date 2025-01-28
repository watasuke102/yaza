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
#include "zwin/gles_v32/rendering_unit.hpp"

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

  void add_rendering_unit(gles_v32::rendering_unit::RenderingUnit* unit);
  void remove_rendering_unit(gles_v32::rendering_unit::RenderingUnit* unit);
  void queue_frame_callback(wl_resource* callback_resource) const;

  void listen_commited(util::Listener<std::nullptr_t*>& listener);

 private:
  struct {
    util::Signal<std::nullptr_t*> committed;
  } events_;

  struct {
    wl_list frame_callback_list;
  } pending_, current_;
  bool committed_  = false;
  bool destroying_ = false;

  std::list<gles_v32::rendering_unit::RenderingUnit*> rendering_unit_list_;
  util::Listener<remote::Session*> session_established_listener_;
  util::Listener<std::nullptr_t*>  session_disconnected_listener_;
  util::Listener<std::nullptr_t*>  session_frame_listener_;
  std::optional<std::unique_ptr<zen::remote::server::IVirtualObject>> proxy_ =
      std::nullopt;
};
void create(wl_client* client, uint32_t id);
}  // namespace yaza::zwin::virtual_object
