#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <wayland-util.h>

#include <exception>

#include "server.hpp"

int main() {
  try {
    yaza::Server server;
    server.start();
  } catch (std::exception& _) {
    return 1;
  }
  return 0;
}
