#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-util.h>

#include <cstdlib>
#include <exception>

#include "common.hpp"
#include "server.hpp"

int main() {
  try {
    yaza::Server server;
    if (server.is_creation_failed()) {
      LOG_ERR("Failed to create server; aborting");
      throw std::exception();
    }
    server.start();
    LOG_ERR("Exiting gracefully");
    return EXIT_SUCCESS;
  } catch (std::exception& e) {
    return EXIT_FAILURE;
  }
}
