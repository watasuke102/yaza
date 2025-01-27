// NOLINTBEGIN(google-readability-casting)
#include "wayland/seat/input_listen_server.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cerrno>
#include <cstdint>
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
    while (!this->terminate_requested_) {
      // wait for client
      sockaddr_in client_addr;
      int client = accept4(this->socket_, nullptr, nullptr, SOCK_CLOEXEC);
      if (client == -1) {
        BAIL("Failed to accept a client");
      }
      LOG_INFO(
          "Connected with input client: %s", inet_ntoa(client_addr.sin_addr));
      this->handle_events(client);
      close(client);
    }
  });

  LOG_INFO("InputListenServer is started (port: %d)", kServerPort);
#undef BAIL
}

InputListenServer::~InputListenServer() {
  // FIXME: this means nothing, proper error handling needed
  // like not using blocking socket API (epoll)
  this->terminate_requested_ = true;
  if (this->socket_ != -1) {
    close(this->socket_);
  }
}

void InputListenServer::handle_events(int client) const {
  constexpr uint32_t kMagic = ('y' << 24) | ('a' << 16) | ('z' << 8) | 'a';
  std::array<uint8_t, 512> buf{0};

  while (!this->terminate_requested_) {
    ssize_t size = recv(client, buf.data(), sizeof(uint8_t) * buf.size(), 0);
    if (size == -1) {
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
    switch (event->type_) {
      case EventType::MOUSE_MOVE:
        constexpr float kDivider = 100.F;
        this->seat_->move_rel_pointing(-event->data_.movement_[1] / kDivider,
            -event->data_.movement_[0] / kDivider);
        printf("[type 1: mouse movement] (x, y) = (%5.4f, %5.4f)\n",
            event->data_.movement_[0], event->data_.movement_[1]);
        break;
    }
  }
}
}  // namespace yaza::wayland::seat

// NOLINTEND(google-readability-casting)
