#pragma once

#include <wayland-server-core.h>
#include <zen-remote/server/gl-sampler.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <variant>

#include "common.hpp"
#include "util/signal.hpp"

namespace yaza::zwin::gles_v32::gl_sampler {
enum class ParamType : uint8_t {
  F,
  I,
  FV,
  IV,
  IIV,
  IUIV,
};

struct Parameter {
  bool                                                  changed;
  ParamType                                             type;
  uint32_t                                              pname;
  std::variant<int32_t, float, std::array<uint8_t, 16>> param;
};

class GlSampler {
 public:
  DISABLE_MOVE_AND_COPY(GlSampler);
  GlSampler();
  ~GlSampler();

  void     commit();
  void     sync(bool force_sync);
  uint64_t remote_id() {
    assert(this->proxy_.has_value());
    return this->proxy_->get()->id();
  }

  void set_paramater(wl_resource* resource, ParamType type, uint32_t pname,
      std::variant<int32_t, float, wl_array*> param);

 private:
  struct {
    std::unordered_map<uint32_t, Parameter> params;
  } pending_, current_;

  util::Listener<std::nullptr_t*> session_disconnected_listener_;
  std::optional<std::unique_ptr<zen::remote::server::IGlSampler>> proxy_ =
      std::nullopt;
};

void create(wl_client* client, uint32_t id);
}  // namespace yaza::zwin::gles_v32::gl_sampler
