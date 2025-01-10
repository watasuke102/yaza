#pragma once

#include <wayland-server-core.h>
#include <zen-remote/server/channel.h>
#include <zen-remote/server/peer.h>
#include <zen-remote/server/session.h>

#include <cstddef>
#include <functional>
#include <memory>

#include "common.hpp"
#include "util/signal.hpp"

namespace yaza::remote {
class Session {
 public:
  DISABLE_MOVE_AND_COPY(Session);
  explicit Session(const std::shared_ptr<zen::remote::server::IPeer>& peer,
      wl_event_loop* wl_loop, std::function<void()> on_disconnect);
  ~Session();
  [[nodiscard]] uint64_t                                       id() const;
  [[nodiscard]] std::shared_ptr<zen::remote::server::IChannel> channel() const;

 private:
  wl_event_loop* wl_loop_;
  uint64_t       peer_id_;

  std::shared_ptr<zen::remote::server::ISession> session_;
  std::shared_ptr<zen::remote::server::IChannel> channel_;
};

void listen_session_established(util::Listener<Session*>& listener);
void listen_session_disconnected(util::Listener<std::nullptr_t*>& listener);
void create(wl_event_loop* loop);
}  // namespace yaza::remote
