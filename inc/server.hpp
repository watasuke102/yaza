#pragma once

#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-util.h>

namespace yaza {
class Server {
 public:
  Server();
  ~Server();
  void start();

  Server(const Server&)            = delete;
  Server(Server&&)                 = delete;
  Server& operator=(const Server&) = delete;
  Server& operator=(Server&&)      = delete;

 private:
  bool        is_started_;
  wl_display* wl_display_;
  const char* socket_;
};
}  // namespace yaza
