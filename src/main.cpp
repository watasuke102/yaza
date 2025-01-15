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
      yaza::Server server;
      if (server.is_creation_failed()) {
        throw std::runtime_error("Failed to create server; aborting");
      }
      server.start();
    }
    LOG_INFO("Exiting gracefully");
    return EXIT_SUCCESS;
  } catch (std::exception& e) {
    LOG_ERR("%s", e.what());
    return EXIT_FAILURE;
  }
}
