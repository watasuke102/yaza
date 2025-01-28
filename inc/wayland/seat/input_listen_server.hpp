#pragma once

#include <cstdint>
#include <thread>

#include "common.hpp"

namespace yaza::wayland::seat {
class Seat;
constexpr uint16_t kServerPort = 22202;

enum class EventType : uint32_t {  // NOLINT
  MOUSE_MOVE = 1,
};

struct Event {
  uint32_t  magic;
  EventType type;
  union {
    float movement[2];
  } data;
};

// owned by Seat
class InputListenServer {
 public:
  DISABLE_MOVE_AND_COPY(InputListenServer);
  explicit InputListenServer(Seat* seat);
  ~InputListenServer();

 private:
  void handle_events(int client) const;

  Seat*       seat_;
  std::thread thread_;
  int         socket_              = -1;
  bool        terminate_requested_ = false;
};
}  // namespace yaza::wayland::seat
