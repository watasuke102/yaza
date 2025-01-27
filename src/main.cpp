#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-util.h>

#include <cstdlib>
#include <exception>
#include <stdexcept>

#include "common.hpp"
#include "server.hpp"

int main() {
  try {
    {
      if (!yaza::server::Server::init()) {
        throw std::runtime_error("Failed to create server");
      }
      yaza::server::get().start();
      yaza::server::get().terminate();
    }
    LOG_INFO("Exiting gracefully");
    return EXIT_SUCCESS;
  } catch (std::exception& e) {
    LOG_ERR("%s", e.what());
    return EXIT_FAILURE;
  }
}
