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
#include "input/bounded_object.hpp"
#include "remote/session.hpp"
#include "renderer.hpp"
#include "util/data_pool.hpp"
#include "util/signal.hpp"

namespace yaza::wayland::surface {
// TODO: CURSOR surface should be owned by Seat?
// (Should be moved from Server)
enum class Role : uint8_t {
  DEFAULT,
  CURSOR,
};

class Surface : public input::BoundedObject {
 public:
  DISABLE_MOVE_AND_COPY(Surface);
  explicit Surface(wl_resource* resource);
  ~Surface();

  void enter(input::IntersectInfo& intersect_info) override;
  void leave() override;
  void motion(input::IntersectInfo& intersect_info) override;
  void button(uint32_t button, wl_pointer_button_state state) override;
  void axis(float amount) override;
  void frame() override;
  std::optional<input::IntersectInfo> check_intersection(
      const glm::vec3& origin, const glm::vec3& direction) override;
  void move(float polar, float azimuthal) override;  // for DEFAULT
  [[nodiscard]] wl_resource* resource() const override {
    return this->resource_;
  }
  [[nodiscard]] wl_client* client() const override {
    return wl_resource_get_client(this->resource_);
  }

  void attach(wl_resource* buffer);
  void queue_frame_callback(wl_resource* resource) const;
  void commit();

  void set_role(Role role);
  void set_offset(glm::ivec2 offset);
  void set_active(bool is_active);
  void move(glm::vec3 pos, glm::quat rot, glm::ivec2 hotspot);  // for CURSOR
  void listen_committed(util::Listener<std::nullptr_t*>& listener);

  Role role() {
    return this->role_;
  };

 private:
  struct {
    util::Signal<std::nullptr_t*> committed;
  } events_;

  struct {
    std::optional<wl_resource*> buffer = std::nullopt;
    wl_list                     frame_callback_list;
    glm::ivec2                  offset         = glm::vec2(0);  // surface local
    bool                        offset_changed = false;
  } pending_;
  struct {
    wl_list frame_callback_list;
  } current_;
  glm::ivec2 offset_    = glm::vec2(0);  // surface local
  bool       is_active_ = true;          // only for CURSOR

  Role role_ = Role::DEFAULT;

  util::DataPool texture_;
  uint32_t       tex_width_;
  uint32_t       tex_height_;
  void           set_texture_size(uint32_t width, uint32_t height);

  float                     polar_     = std::numbers::pi / 2.F;
  float                     azimuthal_ = std::numbers::pi;
  void                      update_pos_and_rot();
  void                      sync_geom();
  std::unique_ptr<Renderer> renderer_;
  void                      init_renderer();

  [[nodiscard]] std::optional<wl_resource*> get_wl_pointer() const;
  util::Listener<remote::Session*>          session_established_listener_;
  util::Listener<std::nullptr_t*>           session_disconnected_listener_;
  util::Listener<std::nullptr_t*>           session_frame_listener_;
  wl_resource*                              resource_;
};

void create(wl_client* client, int version, uint32_t id);
}  // namespace yaza::wayland::surface
