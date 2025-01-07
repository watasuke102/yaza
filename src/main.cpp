#include <wayland-server-core.h>

#include <iostream>

class Server {
 public:
  Server()
      : wl_display_(wl_display_create())
      , socket_(wl_display_add_socket_auto(this->wl_display_)) {
    setenv("WAYLAND_DISPLAY", this->socket_, true);
  }
  ~Server() {
    wl_display_destroy_clients(this->wl_display_);
    wl_display_destroy(this->wl_display_);
  }
  void start() {
    std::cout << "[INFO] server started at " << this->socket_ << '\n';
    wl_display_run(this->wl_display_);
  }

  Server(const Server&)            = delete;
  Server(Server&&)                 = delete;
  Server& operator=(const Server&) = delete;
  Server& operator=(Server&&)      = delete;

 private:
  struct wl_display* wl_display_;
  const char*        socket_;
};

int main() {
  Server server;
  server.start();
  return 0;
}
