#pragma once

#include <wayland-server-core.h>
#include <wayland-server.h>

#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include <memory>
#include <optional>
#include <utility>

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
  explicit Surface(wl_resource* resource);
  ~Surface();

  void attach(wl_resource* buffer, int32_t sx, int32_t sy);
  void set_callback(wl_resource* resource);
  void commit();

  std::optional<std::pair<float, float>> intersected_at(
      const glm::vec3& origin, const glm::vec3& direction);
  void listen_committed(util::Listener<std::nullptr_t*>& listener);

  wl_resource* resource() {
    return this->resource_;
  }
  wl_client* client() {
    return wl_resource_get_client(this->resource_);
  }

 private:
  struct {
    util::Signal<std::nullptr_t*> committed;
  } events_;

  struct {
    std::optional<wl_resource*> buffer   = std::nullopt;
    std::optional<wl_resource*> callback = std::nullopt;
  } pending_;

  util::DataPool texture_;
  uint32_t       tex_width_;
  uint32_t       tex_height_;

  util::Box                 geom_;
  glm::mat4                 geom_mat_;  // use on checking for intersection
  void                      update_geom();
  std::unique_ptr<Renderer> renderer_;
  void                      init_renderer();

  util::Listener<remote::Session*> session_established_listener_;
  util::Listener<std::nullptr_t*>  session_disconnected_listener_;
  wl_resource*                     resource_;
};

void create(wl_client* client, int version, uint32_t id);
}  // namespace yaza::wayland::surface
