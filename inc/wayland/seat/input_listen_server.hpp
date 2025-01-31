#pragma once

#include <cstdint>
#include <thread>

#include "common.hpp"

namespace yaza::wayland::seat {
class Seat;
constexpr uint16_t kServerPort = 22202;

enum class EventType : uint32_t {  // NOLINT
  MOUSE_MOVE = 1,
  MOUSE_DOWN,
  MOUSE_UP,
};

struct Event {
  uint32_t  magic;
  EventType type;
  union {
    float    movement[2];  // MOUSE_MOVE
    uint32_t button;       // MOUSE_{DOWN, UP} (reserved)
  } data;
};

constexpr float kMouseMovementDivider = 150.F;

// owned by Seat
class InputListenServer {
 public:
  DISABLE_MOVE_AND_COPY(InputListenServer);
  explicit InputListenServer();
  ~InputListenServer();

 private:
  void handle_events(int client) const;

  std::thread thread_;
  int         socket_              = -1;
  bool        terminate_requested_ = false;
};
}  // namespace yaza::wayland::seat
