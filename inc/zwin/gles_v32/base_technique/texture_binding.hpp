#pragma once

#include <wayland-server-core.h>
#include <wayland-server.h>
#include <zen-remote/server/gl-base-technique.h>
#include <zwin-gles-v32-protocol.h>

#include <cassert>
#include <cstring>
#include <list>

#include "common.hpp"
#include "util/weakable_unique_ptr.hpp"
#include "zwin/gles_v32/gl_sampler.hpp"
#include "zwin/gles_v32/gl_texture.hpp"

namespace yaza::zwin::gles_v32::gl_base_technique {
struct TextureBinding {
  TextureBinding(uint32_t binding, const char* name, uint32_t target,
      util::WeakPtr<gl_texture::GlTexture>&& texture,
      util::WeakPtr<gl_sampler::GlSampler>&& sampler);

  uint32_t                             binding;
  std::string                          name;
  uint32_t                             target;
  util::WeakPtr<gl_texture::GlTexture> texture;
  util::WeakPtr<gl_sampler::GlSampler> sampler;
};

class TextureBindingList {
 public:
  DISABLE_MOVE_AND_COPY(TextureBindingList);
  TextureBindingList()  = default;
  ~TextureBindingList() = default;

  static void commit(TextureBindingList& pending, TextureBindingList& current);
  void sync(std::unique_ptr<zen::remote::server::IGlBaseTechnique>& proxy,
      bool                                                          force_sync);

  void emplace(uint32_t binding, const char* name, uint32_t target,
      util::WeakPtr<gl_texture::GlTexture>&& texture,
      util::WeakPtr<gl_sampler::GlSampler>&& sampler);

 private:
  std::list<TextureBinding> list_;
  bool                      changed_ = false;

  void remove_expired();
};
}  // namespace yaza::zwin::gles_v32::gl_base_technique
