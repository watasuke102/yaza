#pragma once

#include <wayland-server-core.h>
#include <wayland-server.h>
#include <zen-remote/server/gl-base-technique.h>

#include <cassert>
#include <memory>

#include "common.hpp"
#include "util/signal.hpp"

// Do not include `rendering_unit.hpp` to prevent circular reference
namespace yaza::zwin::gles_v32::rendering_unit {
class RenderingUnit;
}

namespace yaza::zwin::gles_v32::gl_base_technique {
class GlBaseTechnique {
 public:
  DISABLE_MOVE_AND_COPY(GlBaseTechnique);
  explicit GlBaseTechnique(
      wl_resource* resource, rendering_unit::RenderingUnit* unit);
  ~GlBaseTechnique();

  void     commit();
  void     sync(bool force_sync);
  uint64_t remote_id() {
    assert(this->proxy_.has_value());
    return this->proxy_->get()->id();
  }

 private:
  struct {
  } pending_;
  struct {
  } current_;
  bool commited_;

  // nonnull; Note that GlBaseTechnique is destroyed
  // when RenderingUnit is going to be destroyed
  rendering_unit::RenderingUnit*  owner_;
  util::Listener<std::nullptr_t*> owner_committed_listener_;

  wl_resource*                    resource_;
  util::Listener<std::nullptr_t*> session_disconnected_listener_;
  std::optional<std::unique_ptr<zen::remote::server::IGlBaseTechnique>> proxy_ =
      std::nullopt;
};

void create(
    wl_client* client, uint32_t id, rendering_unit::RenderingUnit* unit);
}  // namespace yaza::zwin::gles_v32::gl_base_technique
