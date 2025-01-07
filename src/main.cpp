#include <wayland-server-core.h>
#include <wayland-util.h>

#include <exception>

#include "log.hpp"

class Server {
 public:
  Server() {
    this->wl_display_ = wl_display_create();
    if (!this->wl_display_) {
      LOG_ERR("Failed to create display");
      throw std::exception();
    }
    this->socket_ = wl_display_add_socket_auto(this->wl_display_);
    setenv("WAYLAND_DISPLAY", this->socket_, true);
  }
  ~Server() {
    wl_display_destroy_clients(this->wl_display_);
    wl_display_destroy(this->wl_display_);
  }
  void start() {
    LOG_INFO("server started at %s\n", this->socket_);
    wl_display_run(this->wl_display_);
  }

  Server(const Server&)            = delete;
  Server(Server&&)                 = delete;
  Server& operator=(const Server&) = delete;
  Server& operator=(Server&&)      = delete;

 private:
  wl_display* wl_display_;
  const char* socket_;
};

int main() {
  try {
    Server server;
    server.start();
  } catch (std::exception& _) {
    return 1;
  }
  return 0;
}
