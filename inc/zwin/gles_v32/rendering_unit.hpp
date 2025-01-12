#pragma once

#include <wayland-server-core.h>
#include <wayland-server.h>
#include <zen-remote/server/rendering-unit.h>

#include <cassert>
#include <memory>
#include <optional>

#include "common.hpp"
#include "util/signal.hpp"
#include "zwin/gles_v32/gl_base_technique.hpp"

// Do not include `virtual_object.hpp` to prevent circular reference
namespace yaza::zwin::virtual_object {
class VirtualObject;
}

namespace yaza::zwin::gles_v32::rendering_unit {
class RenderingUnit {
 public:
  DISABLE_MOVE_AND_COPY(RenderingUnit);
  explicit RenderingUnit(
      wl_resource* resource, virtual_object::VirtualObject* virtual_object);
  ~RenderingUnit();

  void     commit();
  void     sync(bool force_sync);
  uint64_t remote_id() {
    assert(this->proxy_.has_value());
    return this->proxy_->get()->id();
  }

  void set_technique(
      std::optional<gl_base_technique::GlBaseTechnique*> technique);

  void listen_commited(util::Listener<std::nullptr_t*>& listener);

 private:
  struct {
    util::Signal<std::nullptr_t*> committed_;
  } events_;

  bool committed_ = false;

  // nonnull; Note that RenderingUnit is destroyed
  // when VirtualObject is going to be destroyed
  virtual_object::VirtualObject*  owner_;
  util::Listener<std::nullptr_t*> owner_committed_listener_;

  std::optional<gl_base_technique::GlBaseTechnique*> technique_;

  wl_resource*                    resource_;
  util::Listener<std::nullptr_t*> session_disconnected_listener_;
  std::optional<std::unique_ptr<zen::remote::server::IRenderingUnit>> proxy_ =
      std::nullopt;
};

void create(wl_client* client, uint32_t id,
    virtual_object::VirtualObject* virtual_object);
}  // namespace yaza::zwin::gles_v32::rendering_unit
