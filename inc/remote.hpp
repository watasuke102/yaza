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
#include "zen-remote/server/peer-manager.h"

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

class Remote {
 public:
  DISABLE_MOVE_AND_COPY(Remote);
  explicit Remote(wl_event_loop* loop);
  ~Remote();

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
extern std::unique_ptr<Remote> g_remote;

void listen_session_established(util::Listener<Session*>& listener);
void listen_session_disconnected(util::Listener<std::nullptr_t*>& listener);
void create(wl_event_loop* loop);
}  // namespace yaza::remote
