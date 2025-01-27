#pragma once

#include <wayland-server-core.h>
#include <wayland-server.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

#include "common.hpp"
#include "remote/session.hpp"
#include "renderer.hpp"
#include "util/box.hpp"
#include "util/data_pool.hpp"
#include "util/signal.hpp"

namespace yaza::wayland::surface {
class Surface {
 public:
  DISABLE_MOVE_AND_COPY(Surface);
  explicit Surface(uint32_t id);
  ~Surface();

  void attach(wl_resource* buffer, int32_t sx, int32_t sy);
  void set_callback(wl_resource* resource);
  void commit();

  void listen_committed(util::Listener<std::nullptr_t*>& listener);

 private:
  struct {
    util::Signal<std::nullptr_t*> committed_;
  } events_;

  struct {
    std::optional<wl_resource*> buffer_   = std::nullopt;
    std::optional<wl_resource*> callback_ = std::nullopt;
  } pending_;

  util::DataPool texture_;
  uint32_t       tex_width_;
  uint32_t       tex_height_;

  util::Box                 geom_;
  void                      update_geom();
  std::unique_ptr<Renderer> renderer_;
  void                      init_renderer();

  util::Listener<remote::Session*> session_established_listener_;
  util::Listener<std::nullptr_t*>  session_disconnected_listener_;
  uint32_t                         id_;
};

void create(wl_client* client, int version, uint32_t id);
}  // namespace yaza::wayland::surface
