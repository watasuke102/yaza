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

#include <vector>

#include "common.hpp"
#include "remote/session.hpp"
#include "util/data_pool.hpp"

namespace yaza {
struct BufferElement {
  float x_, y_, z_;
  float u_, v_;
};

// should be created after Session is established, and
// should be destroyed by owner when Session is disconnected
class Renderer {
 public:
  DISABLE_MOVE_AND_COPY(Renderer);
  Renderer();
  ~Renderer() = default;

  void set_vertex(const std::vector<BufferElement>& buffer);
  void set_texture(util::DataPool& texture, uint32_t width, uint32_t height);
  void request_draw_arrays(uint32_t mode, int32_t first, uint32_t count);
  void commit();

 private:
  std::unique_ptr<zen::remote::server::IVirtualObject>   virtual_object_;
  std::unique_ptr<zen::remote::server::IRenderingUnit>   rendering_unit_;
  std::unique_ptr<zen::remote::server::IGlBaseTechnique> technique_;

  std::unique_ptr<zen::remote::server::IGlShader>  vert_shader_;
  std::unique_ptr<zen::remote::server::IGlShader>  frag_shader_;
  std::unique_ptr<zen::remote::server::IGlProgram> program_;

  std::unique_ptr<zen::remote::server::IGlVertexArray> vert_array_;
  std::unique_ptr<zen::remote::server::IGlBuffer>      vert_buffer_;
  util::DataPool                                       vert_data_;

  std::unique_ptr<zen::remote::server::IGlTexture> texture_;
  std::unique_ptr<zen::remote::server::IGlSampler> sampler_;
  util::DataPool                                   texture_data_;
};
}  // namespace yaza
