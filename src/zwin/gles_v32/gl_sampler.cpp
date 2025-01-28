#include "zwin/gles_v32/gl_sampler.hpp"

#include <GLES3/gl32.h>
#include <wayland-server-core.h>
#include <wayland-server.h>
#include <wayland-util.h>
#include <zen-remote/server/gl-sampler.h>
#include <zwin-gles-v32-protocol.h>
#include <zwin-protocol.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <optional>
#include <stdexcept>
#include <variant>

#include "common.hpp"
#include "remote/remote.hpp"
#include "server.hpp"
#include "util/convert.hpp"
#include "util/visitor_list.hpp"
#include "util/weakable_unique_ptr.hpp"

namespace yaza::zwin::gles_v32::gl_sampler {
namespace {
// NOLINTNEXTLINE(cert-err58-cpp)
const std::unordered_map<uint32_t, size_t> parameter_size_map = {
    {GL_TEXTURE_MIN_FILTER,   4 },
    {GL_TEXTURE_MAG_FILTER,   4 },
    {GL_TEXTURE_WRAP_S,       4 },
    {GL_TEXTURE_WRAP_T,       4 },
    {GL_TEXTURE_WRAP_R,       4 },
    {GL_TEXTURE_MIN_LOD,      4 },
    {GL_TEXTURE_MAX_LOD,      4 },
    {GL_TEXTURE_COMPARE_MODE, 4 },
    {GL_TEXTURE_COMPARE_FUNC, 4 },
    {GL_TEXTURE_BORDER_COLOR, 16}
};
}  // namespace

GlSampler::GlSampler() {
  this->session_disconnected_listener_.set_handler(
      [this](std::nullptr_t* /*data*/) {
        this->proxy_ = std::nullopt;
      });
  server::get().remote->listen_session_disconnected(
      this->session_disconnected_listener_);
  LOG_DEBUG("created: GlSampler");
}
GlSampler::~GlSampler() {
  LOG_DEBUG("destructor: GlSampler");
}

void GlSampler::commit() {
  for (auto& [pname, param] : this->pending_.params) {
    this->current_.params.insert_or_assign(pname, param);
  }
  this->pending_.params.clear();
}

void GlSampler::sync(bool force_sync) {
  if (!this->proxy_.has_value()) {
    this->proxy_ = zen::remote::server::CreateGlSampler(
        server::get().remote->channel_nonnull());
  }
  for (auto& [pname, param] : this->current_.params) {
    if (!force_sync && !param.changed) {
      continue;
    }
    // NOLINTBEGIN(google-readability-casting)
    switch (param.type) {
      case ParamType::F: {
        auto value = std::get<float>(param.param);
        this->proxy_->get()->GlSamplerParameterf(pname, value);
        break;
      }
      case ParamType::I: {
        auto value = std::get<int32_t>(param.param);
        this->proxy_->get()->GlSamplerParameteri(pname, value);
        break;
      }
      case ParamType::FV: {
        auto& value = std::get<std::array<uint8_t, 16>>(param.param);
        this->proxy_->get()->GlSamplerParameterfv(pname, (float*)value.data());
        break;
      }
      case ParamType::IV: {
        auto& value = std::get<std::array<uint8_t, 16>>(param.param);
        this->proxy_->get()->GlSamplerParameteriv(
            pname, (int32_t*)value.data());
        break;
      }
      case ParamType::IIV: {
        auto& value = std::get<std::array<uint8_t, 16>>(param.param);
        this->proxy_->get()->GlSamplerParameterIiv(
            pname, (int32_t*)value.data());
        break;
      }
      case ParamType::IUIV: {
        auto& value = std::get<std::array<uint8_t, 16>>(param.param);
        this->proxy_->get()->GlSamplerParameterIuiv(
            pname, (uint32_t*)value.data());
        break;
      }
    }
    // NOLINTEND(google-readability-casting)
  }
}

void GlSampler::set_paramater(wl_resource* resource, ParamType type,
    uint32_t pname, std::variant<int32_t, float, wl_array*> param) {
  {  // valiation block
    size_t expected_size = 0;
    try {
      expected_size = parameter_size_map.at(pname);
    } catch (std::out_of_range&) {
      wl_resource_post_error(resource, ZWN_GLES_V32_ERROR_INVALID_ENUM,
          "invalid pname: %d", pname);
      return;
    }

    const auto size_err = [resource](size_t expect, size_t actual) {
      wl_resource_post_error(resource, ZWN_COMPOSITOR_ERROR_WL_ARRAY_SIZE,
          "expect: %ld, actual: %ld", (expect), (actual));
    };
    if (std::holds_alternative<wl_array*>(param)) {
      auto* array = std::get<wl_array*>(param);
      if (array->size != expected_size) {
        size_err(expected_size, array->size);
        return;
      }
    } else {
      size_t actual_size = std::visit(
          [](auto&& arg) {
            return sizeof(decltype(arg));
          },
          param);
      if (actual_size != expected_size) {
        size_err(expected_size, actual_size);
        return;
      }
    }
  }  // validation succeeded
  Parameter pending{
      .changed = true,
      .type    = type,
      .pname   = pname,
      .param   = 0,  // tmp
  };
  util::VisitorList(
      [&pending](int32_t& param) {
        pending.param.emplace<int32_t>(param);
      },
      [&pending](float& param) {
        pending.param.emplace<float>(param);
      },
      [&pending](wl_array*& param) {
        std::array<uint8_t, 16> data;
        std::memcpy(data.data(), param->data, param->size);
        pending.param.emplace<std::array<uint8_t, 16>>(data);
      })
      .visit(param);
  this->pending_.params.insert_or_assign(pname, pending);
}

namespace {
void destroy(wl_client* /*client*/, wl_resource* resource) {
  wl_resource_destroy(resource);
}
void parameter_f(wl_client* /*client*/, wl_resource* resource, uint32_t pname,
    wl_array* param_array) {
  float param = 0.F;
  if (!util::convert_wl_array(param_array, &param)) {
    wl_resource_post_error(resource, ZWN_COMPOSITOR_ERROR_WL_ARRAY_SIZE,
        "paramater size (%ld) does not equal float size (%ld)",
        param_array->size, sizeof(float));
    return;
  }
  auto* self = static_cast<util::UniPtr<GlSampler>*>(
      wl_resource_get_user_data(resource));
  (*self)->set_paramater(resource, ParamType::F, pname, param);
}
void parameter_i(wl_client* /*client*/, wl_resource* resource, uint32_t pname,
    int32_t param) {
  auto* self = static_cast<util::UniPtr<GlSampler>*>(
      wl_resource_get_user_data(resource));
  (*self)->set_paramater(resource, ParamType::I, pname, param);
}
void parameter_fv(wl_client* /*client*/, wl_resource* resource, uint32_t pname,
    wl_array* params) {
  auto* self = static_cast<util::UniPtr<GlSampler>*>(
      wl_resource_get_user_data(resource));
  (*self)->set_paramater(resource, ParamType::FV, pname, params);
}
void parameter_iv(wl_client* /*client*/, wl_resource* resource, uint32_t pname,
    wl_array* params) {
  auto* self = static_cast<util::UniPtr<GlSampler>*>(
      wl_resource_get_user_data(resource));
  (*self)->set_paramater(resource, ParamType::IV, pname, params);
}
void parameter_iiv(wl_client* /*client*/, wl_resource* resource, uint32_t pname,
    wl_array* params) {
  auto* self = static_cast<util::UniPtr<GlSampler>*>(
      wl_resource_get_user_data(resource));
  (*self)->set_paramater(resource, ParamType::IIV, pname, params);
}
void parameter_iuiv(wl_client* /*client*/, wl_resource* resource,
    uint32_t pname, wl_array* params) {
  auto* self = static_cast<util::UniPtr<GlSampler>*>(
      wl_resource_get_user_data(resource));
  (*self)->set_paramater(resource, ParamType::IUIV, pname, params);
}
constexpr struct zwn_gl_sampler_interface kImpl = {
    .destroy       = destroy,
    .parameterf    = parameter_f,
    .parameteri    = parameter_i,
    .parameterfv   = parameter_fv,
    .parameteriv   = parameter_iv,
    .parameterIiv  = parameter_iiv,
    .parameterIuiv = parameter_iuiv,
};

void destroy(wl_resource* resource) {
  auto* self = static_cast<util::UniPtr<GlSampler>*>(
      wl_resource_get_user_data(resource));
  delete self;
}
}  // namespace
void create(wl_client* client, uint32_t id) {
  wl_resource* resource =
      wl_resource_create(client, &zwn_gl_sampler_interface, 1, id);
  if (!resource) {
    wl_client_post_no_memory(client);
    return;
  }
  auto* self = new util::UniPtr<GlSampler>();
  wl_resource_set_implementation(resource, &kImpl, self, destroy);
}
}  // namespace yaza::zwin::gles_v32::gl_sampler
