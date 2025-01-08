#pragma once

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-util.h>

#include "common.hpp"

namespace yaza {
class Server {
 public:
  DISABLE_MOVE_AND_COPY(Server);
  Server();
  ~Server();
  void start();

 private:
  bool        is_started_;
  const char* socket_;

  wl_display* wl_display_;
  wl_global*  compositor_;
  wl_global*  xdg_wm_base_;
  wl_global*  zwin_compositor_;
  wl_global*  zwin_seat_;
  wl_global*  zwin_shell_;
  wl_global*  zwin_shm_;
  wl_global*  zwin_gles_v32_;
};
}  // namespace yaza
