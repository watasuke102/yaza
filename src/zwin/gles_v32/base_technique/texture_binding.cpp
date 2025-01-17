#include "zwin/gles_v32/base_technique/texture_binding.hpp"

namespace yaza::zwin::gles_v32::gl_base_technique {
TextureBinding::TextureBinding(uint32_t binding, const char* name,
    uint32_t target, util::WeakPtr<gl_texture::GlTexture>&& texture,
    util::WeakPtr<gl_sampler::GlSampler>&& sampler)
    : binding_(binding)
    , name_(name)
    , target_(target)
    , texture_(std::move(texture))
    , sampler_(std::move(sampler)) {
}

void TextureBindingList::commit(
    TextureBindingList& pending, TextureBindingList& current) {
  // ensure that WeakPtr of texture and sample are valid
  pending.remove_expired();

  current.changed_ = pending.changed_;
  if (pending.changed_) {
    current.list_.clear();
    current.list_.insert(
        current.list_.end(), pending.list_.begin(), pending.list_.end());
    pending.changed_ = false;
  } else {
    current.remove_expired();  // ensure for current
  }

  for (auto& current : current.list_) {
    current.texture_.lock()->commit();
    current.sampler_.lock()->commit();
  }
}
void TextureBindingList::sync(
    std::unique_ptr<zen::remote::server::IGlBaseTechnique>& proxy,
    bool                                                    force_sync) {
  this->remove_expired();
  for (auto& binding : this->list_) {
    auto* texture = binding.texture_.lock();
    auto* sampler = binding.sampler_.lock();
    texture->sync(force_sync);
    sampler->sync(force_sync);
    if (force_sync || this->changed_) {
      proxy->BindTexture(binding.binding_, binding.name_, binding.target_,
          texture->remote_id(), sampler->remote_id());
    }
  }
}

void TextureBindingList::emplace(uint32_t binding, const char* name,
    uint32_t target, util::WeakPtr<gl_texture::GlTexture>&& texture,
    util::WeakPtr<gl_sampler::GlSampler>&& sampler) {
  this->list_.remove_if([binding](const TextureBinding& elem) {
    return elem.binding_ == binding;
  });
  this->list_.emplace_back(
      binding, name, target, std::move(texture), std::move(sampler));
}

void TextureBindingList::remove_expired() {
  this->list_.remove_if([](const TextureBinding& elem) {
    return !elem.texture_.lock() || !elem.sampler_.lock();
  });
}
}  // namespace yaza::zwin::gles_v32::gl_base_technique
