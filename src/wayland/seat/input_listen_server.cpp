// NOLINTBEGIN(google-readability-casting)
#include "wayland/seat/input_listen_server.hpp"

#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/fcntl.h>
#include <sys/socket.h>

#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>

#include "common.hpp"
#include "wayland/seat/seat.hpp"

namespace yaza::wayland::seat {
InputListenServer::InputListenServer(Seat* seat) : seat_(seat) {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define BAIL(err_prefix)                                                       \
  LOG_ERR(err_prefix ": %s", std::strerror(errno));                            \
  return

  this->socket_ = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (this->socket_ == -1) {
    BAIL("Failed to create a socket");
  }
  if (fcntl(this->socket_, F_SETFL, O_NONBLOCK) == -1) {
    BAIL("Failed to set O_NONBLOCK to socket");
  }
  int opt = 1;
  if (setsockopt(this->socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    LOG_WARN("Failed set SO_REUSEADDR to socket: %s", std::strerror(errno));
  }
  sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family      = AF_INET;
  addr.sin_port        = htons(kServerPort);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(this->socket_, (const sockaddr*)&addr, sizeof(addr)) == -1) {
    BAIL("Failed to bind a socket");
  }
  if (listen(this->socket_, 0) == -1) {
    BAIL("Failed to listen a socket");
  }

  this->thread_ = std::thread([this] {
    {
      sigset_t mask;
      sigemptyset(&mask);
      sigaddset(&mask, SIGINT);
      pthread_sigmask(SIG_BLOCK, &mask, nullptr);
    }

    while (!this->terminate_requested_) {
      sockaddr_in client_addr;
      memset(&client_addr, 0, sizeof(client_addr));
      socklen_t len = 0;
      int       client =
          accept4(this->socket_, (sockaddr*)&client_addr, &len, SOCK_CLOEXEC);
      if (client == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          std::this_thread::sleep_for(std::chrono::milliseconds(500));
          continue;
        }
        BAIL("Failed to accept a client");
      }
      if (fcntl(client, F_SETFL, O_NONBLOCK) == -1) {
        LOG_WARN("Failed to set O_NONBLOCK to client");
        close(client);
        continue;
      }
      LOG_INFO(
          "Connected with input client: %s", inet_ntoa(client_addr.sin_addr));
      this->handle_events(client);
      if (close(client) == -1) {
        LOG_WARN("Failed to close client properly: %s", std::strerror(errno));
      }
      LOG_INFO("Disconnected with %s", inet_ntoa(client_addr.sin_addr));
    }
    if (close(this->socket_) == -1) {
      LOG_WARN("Failed to close socket properly: %s", std::strerror(errno));
    }
  });

  LOG_INFO("InputListenServer is started (port: %d)", kServerPort);
}

InputListenServer::~InputListenServer() {
  this->terminate_requested_ = true;
  if (this->thread_.joinable()) {
    this->thread_.join();
  }
}

void InputListenServer::handle_events(int client) const {
  constexpr uint32_t kMagic = ('y' << 24) | ('a' << 16) | ('z' << 8) | 'a';
  std::array<uint8_t, 512> buf{0};

  while (!this->terminate_requested_) {
    ssize_t size = recv(client, buf.data(), sizeof(uint8_t) * buf.size(), 0);
    if (size == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        continue;
      }
      LOG_WARN("Failed to receive data from client: %s", strerror(errno));
      break;
    }
    if (size == 0) {
      LOG_WARN("Connection is closed (data size was 0)");
      break;
    }
    if (size < (ssize_t)sizeof(kMagic)) {
      continue;
    }
    uint32_t received_magic = ((uint32_t)buf[0] << 24) |  //
                              ((uint32_t)buf[1] << 16) |  //
                              ((uint32_t)buf[2] << 8) |   //
                              (uint32_t)buf[3];
    if (received_magic != kMagic) {
      LOG_WARN("received an invalid data (magic -> expect: 0x%x, actual: 0x%x)",
          kMagic, received_magic);
      continue;
    }

    auto* event = (Event*)buf.data();
    switch (event->type) {
      case EventType::MOUSE_MOVE:
        constexpr float kDivider = 100.F;
        this->seat_->move_rel_pointing(-event->data.movement[1] / kDivider,
            -event->data.movement[0] / kDivider);
        break;
    }
  }
}
}  // namespace yaza::wayland::seat

// NOLINTEND(google-readability-casting)
