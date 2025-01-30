#include "renderer.hpp"

#include <GLES3/gl32.h>
#include <sys/types.h>
#include <zen-remote/server/gl-buffer.h>
#include <zen-remote/server/gl-sampler.h>
#include <zen-remote/server/gl-shader.h>
#include <zen-remote/server/gl-vertex-array.h>

#include <glm/ext/quaternion_float.hpp>
#include <memory>

#include "server.hpp"

namespace yaza {
Buffer::Buffer(int32_t size, uint32_t type, const void* data, ssize_t data_size)
    : size_(size), type_(type) {
  auto channel  = server::get().remote->channel_nonnull();
  this->buffer_ = zen::remote::server::CreateGlBuffer(channel);
  if (data) {
    this->set_data(data, data_size);
  }
}
void Buffer::set_data(const void* data, ssize_t data_size) {
  this->data_.from_ptr(data, data_size);
  this->buffer_->GlBufferData(
      this->data_.create_buffer(), GL_ARRAY_BUFFER, data_size, GL_STATIC_DRAW);
}

Renderer::Renderer(const char* vert_shader, const char* frag_shader)
    : pos_(0.F), rot_() {
  auto channel          = server::get().remote->channel_nonnull();
  this->virtual_object_ = zen::remote::server::CreateVirtualObject(channel);
  this->rendering_unit_ = zen::remote::server::CreateRenderingUnit(
      channel, this->virtual_object_->id());
  this->technique_ = zen::remote::server::CreateGlBaseTechnique(
      channel, this->rendering_unit_->id());
  this->vert_shader_ = zen::remote::server::CreateGlShader(
      channel, vert_shader, GL_VERTEX_SHADER);
  this->frag_shader_ = zen::remote::server::CreateGlShader(
      channel, frag_shader, GL_FRAGMENT_SHADER);
  this->program_    = zen::remote::server::CreateGlProgram(channel);
  this->vert_array_ = zen::remote::server::CreateGlVertexArray(channel);
  this->texture_    = zen::remote::server::CreateGlTexture(channel);
  this->sampler_    = zen::remote::server::CreateGlSampler(channel);

  this->program_->GlAttachShader(this->vert_shader_->id());
  this->program_->GlAttachShader(this->frag_shader_->id());
  this->program_->GlLinkProgram();

  this->technique_->BindProgram(this->program_->id());
  this->technique_->BindVertexArray(this->vert_array_->id());
}

void Renderer::move_abs(float x, float y, float z) {
  this->pos_.x = x;
  this->pos_.y = y;
  this->pos_.z = z;
}
void Renderer::move_abs(glm::vec3& v) {
  this->pos_ = v;
}
void Renderer::set_rot(glm::quat& q) {
  this->rot_ = q;
}

void Renderer::register_buffer(uint32_t index, int32_t size, uint32_t type,
    const void* data, ssize_t data_size) {
  auto [it, inserted] =
      this->buffers_.try_emplace(index, size, type, data, data_size);
  if (!inserted) {
    return;
  }
  this->vert_array_->GlEnableVertexAttribArray(index);
  this->vert_array_->GlVertexAttribPointer(
      index, size, type, GL_FALSE, 0, 0, it->second.buffer_id());
}
void Renderer::set_texture(
    util::DataPool& texture, uint32_t width, uint32_t height) {
  this->texture_->GlTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
      GL_RGBA, GL_UNSIGNED_BYTE, texture.create_buffer());
  this->texture_->GlGenerateMipmap(GL_TEXTURE_2D);
  this->sampler_->GlSamplerParameteri(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  this->sampler_->GlSamplerParameteri(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  this->technique_->BindTexture(
      0, "", this->texture_->id(), GL_TEXTURE_2D, this->sampler_->id());
}
void Renderer::set_uniform_matrix(
    uint32_t location, const char* name, glm::mat4& mat) {
  this->technique_->GlUniformMatrix(
      location, name, 4, 4, 1, false, (float*)&mat);  // NOLINT
}
void Renderer::request_draw_arrays(
    uint32_t mode, int32_t first, uint32_t count) {
  this->technique_->GlDrawArrays(mode, first, count);
}
void Renderer::commit() {
  this->virtual_object_->Move(
      static_cast<float*>(&this->pos_.x), static_cast<float*>(&this->rot_.x));
  this->virtual_object_->Commit();
}
}  // namespace yaza
