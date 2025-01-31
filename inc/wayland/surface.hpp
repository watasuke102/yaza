#pragma once

#include <wayland-server-core.h>
#include <wayland-server.h>

#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_int2.hpp>
#include <memory>
#include <numbers>
#include <optional>

#include "common.hpp"
#include "remote/session.hpp"
#include "renderer.hpp"
#include "util/box.hpp"
#include "util/data_pool.hpp"
#include "util/signal.hpp"

namespace yaza::wayland::surface {
// TODO: CURSOR surface should be owned by Seat?
// (Should be moved from Server)
enum class Role : uint8_t {
  DEFAULT,
  CURSOR,
};

struct SurfaceIntersectInfo {
  float distance;
  float sx, sy;  // surface-local coordinate
};

class Surface {
 public:
  DISABLE_MOVE_AND_COPY(Surface);
  explicit Surface(wl_resource* resource);
  ~Surface();

  void attach(wl_resource* buffer);
  void set_callback(wl_resource* resource);
  void commit();

  void set_role(Role role);
  void set_offset(glm::ivec2 offset);
  void move(float polar, float azimuthal);                      // for DEFAULT
  void move(glm::vec3 pos, glm::quat rot, glm::ivec2 hotspot);  // for CURSOR
  std::optional<SurfaceIntersectInfo> intersected_at(
      const glm::vec3& origin, const glm::vec3& direction);
  void listen_committed(util::Listener<std::nullptr_t*>& listener);

  Role role() {
    return this->role_;
  };
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
    glm::ivec2                  offset   = glm::vec2(0);  // surface local
  } pending_;
  glm::ivec2 offset_ = glm::vec2(0);  // surface local

  Role role_;

  util::DataPool texture_;
  uint32_t       tex_width_;
  uint32_t       tex_height_;
  void           set_texture_size(uint32_t width, uint32_t height);

  float                     polar_     = std::numbers::pi / 2.F;
  float                     azimuthal_ = std::numbers::pi;
  util::Box                 geom_;
  glm::mat4                 geom_mat_;  // use on checking for intersection
  void                      update_pos_and_rot();
  void                      sync_geom();
  std::unique_ptr<Renderer> renderer_;
  void                      init_renderer();

  util::Listener<remote::Session*> session_established_listener_;
  util::Listener<std::nullptr_t*>  session_disconnected_listener_;
  wl_resource*                     resource_;
};

void create(wl_client* client, int version, uint32_t id);
}  // namespace yaza::wayland::surface
