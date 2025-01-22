#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-util.h>

#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <vector>

#include "common.hpp"
#include "server.hpp"

namespace {
yaza::server::Server* g_server;
}
namespace yaza::server {
uint32_t next_serial() {
  return g_server->next_serial();
}
wl_event_loop* loop() {
  return g_server->loop();
}
}  // namespace yaza::server

int main() {
  try {
    {
      g_server = new yaza::server::Server();
      if (g_server->is_creation_failed()) {
        throw std::runtime_error("Failed to create server; aborting");
      }
      g_server->start();
      delete g_server;
    }
    LOG_INFO("Exiting gracefully");
    return EXIT_SUCCESS;
  } catch (std::exception& e) {
    LOG_ERR("%s", e.what());
    return EXIT_FAILURE;
  }
}
