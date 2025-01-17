#pragma once

#include <wayland-server-core.h>
#include <zen-remote/server/gl-texture.h>

#include <cassert>
#include <cstdint>
#include <memory>

#include "common.hpp"
#include "util/data_pool.hpp"
#include "util/signal.hpp"
#include "util/weak_resource.hpp"

namespace yaza::zwin::gles_v32::gl_texture {
struct Image2dData {
  uint32_t target_;
  int32_t  level_;
  int32_t  internal_format_;
  uint32_t width_;
  uint32_t height_;
  int32_t  border_;
  uint32_t format_;
  uint32_t type_;
};

class GlTexture {
 public:
  DISABLE_MOVE_AND_COPY(GlTexture);
  explicit GlTexture(wl_event_loop* loop);
  ~GlTexture();

  void     commit();
  void     sync(bool force_sync);
  uint64_t remote_id() {
    assert(this->proxy_.has_value());
    return this->proxy_->get()->id();
  }

  void new_image_2d(Image2dData& image_2d, wl_resource* data);
  void request_generate_mipmap(uint32_t target);

 private:
  struct {
    Image2dData               image_2d_;
    util::WeakResource<void*> data_;
    uint32_t                  mipmap_target_ = 0;
  } pending_;
  struct {
    Image2dData    image_2d_;
    util::DataPool data_;
    uint32_t       mipmap_target_;
    bool           data_changed_          = false;
    bool           mipmap_target_changed_ = false;
  } current_;

  wl_event_loop*                  loop_;
  util::Listener<std::nullptr_t*> session_disconnected_listener_;
  std::optional<std::unique_ptr<zen::remote::server::IGlTexture>> proxy_ =
      std::nullopt;
};

void create(wl_client* client, uint32_t id, wl_event_loop* loop);
}  // namespace yaza::zwin::gles_v32::gl_texture
