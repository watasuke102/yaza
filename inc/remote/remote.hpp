#pragma once

#include <wayland-server-core.h>
#include <zen-remote/server/channel.h>
#include <zen-remote/server/peer-manager.h>
#include <zen-remote/server/peer.h>
#include <zen-remote/server/session.h>

#include <chrono>
#include <cstddef>
#include <memory>

#include "common.hpp"
#include "remote/session.hpp"
#include "util/signal.hpp"

namespace yaza::remote {
class Remote {
 public:
  DISABLE_MOVE_AND_COPY(Remote);
  explicit Remote(wl_event_loop* loop);
  ~Remote();

  [[nodiscard]] bool                             has_session();
  /// should be called while the session is available
  std::shared_ptr<zen::remote::server::IChannel> channel_nonnull();

  void listen_session_established(util::Listener<Session*>& listener);
  void listen_session_disconnected(util::Listener<std::nullptr_t*>& listener);
  void listen_session_frame(util::Listener<std::nullptr_t*>& listener);

 private:
  wl_event_loop* wl_loop_;
  struct {
    util::Signal<Session*>        session_established_;
    util::Signal<std::nullptr_t*> session_disconnected_;
    util::Signal<std::nullptr_t*> session_frame_;
  } events_;

  std::unique_ptr<zen::remote::Signal<void(uint64_t)>::Connection>
      peer_discover_signal_disconnector_;
  std::unique_ptr<zen::remote::Signal<void(uint64_t)>::Connection>
      peer_lost_signal_disconnector_;

  std::optional<std::unique_ptr<Session>>            current_session_;
  std::unique_ptr<zen::remote::server::IPeerManager> peer_manager_;
  std::chrono::steady_clock::time_point              prev_frame_;
  wl_event_source*                                   frame_timer_source_;

  void disconnect();
};
}  // namespace yaza::remote
