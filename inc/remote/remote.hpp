#pragma once

#include <wayland-server-core.h>
#include <zen-remote/server/channel.h>
#include <zen-remote/server/peer-manager.h>
#include <zen-remote/server/peer.h>
#include <zen-remote/server/session.h>

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

  /// should be called while the session is available
  std::shared_ptr<zen::remote::server::IChannel> channel_nonnull();

  void listen_session_established(util::Listener<Session*>& listener);
  void listen_session_disconnected(util::Listener<std::nullptr_t*>& listener);

 private:
  wl_event_loop* wl_loop_;
  struct {
    util::Signal<Session*>        session_established_;   // data=Session
    util::Signal<std::nullptr_t*> session_disconnected_;  // data=nullptr
  } events_;

  std::optional<std::unique_ptr<Session>>            current_session_;
  std::unique_ptr<zen::remote::server::IPeerManager> peer_manager_;

  void disconnect();
};
extern std::unique_ptr<Remote> g_remote;  // NOLINT

void terminate();
void init(wl_event_loop* loop);
}  // namespace yaza::remote
