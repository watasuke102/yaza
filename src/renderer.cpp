#include "renderer.hpp"

#include <GLES3/gl32.h>
#include <zen-remote/server/gl-buffer.h>
#include <zen-remote/server/gl-sampler.h>
#include <zen-remote/server/gl-shader.h>
#include <zen-remote/server/gl-vertex-array.h>

#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/vector_float3.hpp>
#include <memory>

#include "common.hpp"
#include "remote/remote.hpp"

namespace yaza {
namespace {
#define GLSL(s) (const char*)"#version 320 es\nprecision mediump float;\n" #s
// clang-format off
constexpr auto* kVertShader = GLSL(
  uniform mat4 zMVP;
  layout(location = 0) in vec3 pos;
  layout(location = 1) in vec2 vertex_uv;

  out vec2 uv;

  void main() {
    gl_Position = zMVP * vec4(pos, 1.0);
    uv          = vertex_uv;
  }
);
constexpr auto* kFragShader = GLSL(
  uniform sampler2D texture;
  in  vec2 uv;
  out vec4 color;

  void main() {
    color = texture(texture, uv);
  }
);
// clang-format on
}  // namespace

Renderer::Renderer() {
  auto channel          = remote::g_remote->channel_nonnull();
  this->virtual_object_ = zen::remote::server::CreateVirtualObject(channel);
  this->rendering_unit_ = zen::remote::server::CreateRenderingUnit(
      channel, this->virtual_object_->id());
  this->technique_ = zen::remote::server::CreateGlBaseTechnique(
      channel, this->rendering_unit_->id());
  this->vert_shader_ = zen::remote::server::CreateGlShader(
      channel, kVertShader, GL_VERTEX_SHADER);
  this->frag_shader_ = zen::remote::server::CreateGlShader(
      channel, kFragShader, GL_FRAGMENT_SHADER);
  this->program_     = zen::remote::server::CreateGlProgram(channel);
  this->vert_array_  = zen::remote::server::CreateGlVertexArray(channel);
  this->vert_buffer_ = zen::remote::server::CreateGlBuffer(channel);
  this->texture_     = zen::remote::server::CreateGlTexture(channel);
  this->sampler_     = zen::remote::server::CreateGlSampler(channel);

  this->program_->GlAttachShader(this->vert_shader_->id());
  this->program_->GlAttachShader(this->frag_shader_->id());
  this->program_->GlLinkProgram();

  this->technique_->BindProgram(this->program_->id());
  this->technique_->BindVertexArray(this->vert_array_->id());
  {
    auto v = glm::vec3{0.F, 1.F, -2.F};
    auto q = glm::quat();
    this->virtual_object_->Move(
        static_cast<float*>(&v.x), static_cast<float*>(&q.x));
  }
}

void Renderer::set_vertex(const std::vector<BufferElement>& buffer) {
  if (this->vert_data_.has_data()) {
    this->vert_data_.reset();
  }
  auto size = buffer.size() * sizeof(BufferElement);
  this->vert_data_.from_ptr(buffer.data(), size);
  this->vert_buffer_->GlBufferData(
      this->vert_data_.create_buffer(), GL_ARRAY_BUFFER, size, GL_STATIC_DRAW);
  // location = 0: position
  this->vert_array_->GlEnableVertexAttribArray(0);
  this->vert_array_->GlVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
      sizeof(BufferElement), 0, this->vert_buffer_->id());
  // location = 1: uv
  this->vert_array_->GlEnableVertexAttribArray(1);
  this->vert_array_->GlVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
      sizeof(BufferElement), sizeof(float) * 3, this->vert_buffer_->id());
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
void Renderer::request_draw_arrays(
    uint32_t mode, int32_t first, uint32_t count) {
  this->technique_->GlDrawArrays(mode, first, count);
}
void Renderer::commit() {
  this->virtual_object_->Commit();
}
}  // namespace yaza
