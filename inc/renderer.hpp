#pragma once

#include <zen-remote/server/gl-base-technique.h>
#include <zen-remote/server/gl-buffer.h>
#include <zen-remote/server/gl-program.h>
#include <zen-remote/server/gl-sampler.h>
#include <zen-remote/server/gl-shader.h>
#include <zen-remote/server/gl-texture.h>
#include <zen-remote/server/gl-vertex-array.h>
#include <zen-remote/server/rendering-unit.h>
#include <zen-remote/server/virtual-object.h>

#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/vector_float3.hpp>
#include <unordered_map>

#include "common.hpp"
#include "util/data_pool.hpp"

namespace yaza {
class Buffer {
 public:
  Buffer(int32_t size, uint32_t type, const void* data, ssize_t data_size);
  void     set_data(const void* data, ssize_t data_size);
  uint64_t buffer_id() {
    return buffer_->id();
  }

 private:
  int32_t                                         size_;
  uint32_t                                        type_;
  std::unique_ptr<zen::remote::server::IGlBuffer> buffer_;
  util::DataPool                                  data_;
};

// should be created after Session is established, and
// should be destroyed by owner when Session is disconnected
class Renderer {
 public:
  DISABLE_MOVE_AND_COPY(Renderer);
  Renderer(const char* vert_shader, const char* frag_shader);
  ~Renderer() = default;

  void move_abs(float x, float y, float z);
  void move_abs(glm::vec3& v);
  void set_rot(glm::quat& q);

  void register_buffer(uint32_t index, int32_t size, uint32_t type,
      const void* data, ssize_t data_size);
  void set_texture(util::DataPool& texture, uint32_t width, uint32_t height);
  void set_uniform_matrix(uint32_t location, const char* name, glm::mat4& mat);
  void request_draw_arrays(uint32_t mode, int32_t first, uint32_t count);
  void commit();

 private:
  glm::vec3 pos_;
  glm::quat rot_;

  std::unique_ptr<zen::remote::server::IVirtualObject>   virtual_object_;
  std::unique_ptr<zen::remote::server::IRenderingUnit>   rendering_unit_;
  std::unique_ptr<zen::remote::server::IGlBaseTechnique> technique_;

  std::unique_ptr<zen::remote::server::IGlShader>  vert_shader_;
  std::unique_ptr<zen::remote::server::IGlShader>  frag_shader_;
  std::unique_ptr<zen::remote::server::IGlProgram> program_;

  std::unordered_map<uint32_t, Buffer>                 buffers_;
  std::unique_ptr<zen::remote::server::IGlVertexArray> vert_array_;

  std::unique_ptr<zen::remote::server::IGlTexture> texture_;
  std::unique_ptr<zen::remote::server::IGlSampler> sampler_;
  util::DataPool                                   texture_data_;
};
}  // namespace yaza
