#include "remote/session.hpp"

#include "remote/loop.hpp"

namespace yaza::remote {
Session::Session(const std::shared_ptr<zen::remote::server::IPeer>& peer,
    wl_event_loop* wl_loop, std::function<void()> on_disconnect)
    : wl_loop_(wl_loop)
    , peer_id_(peer->id())
    , session_(zen::remote::server::CreateSession(
          std::make_unique<Loop>(this->wl_loop_))) {
  if (!this->session_->Connect(peer)) {
    LOG_WARN("Failed to connect with peer %lu", peer->id());
    throw std::exception();
  }
  // FIXME: calling on_disconnect() in zen-remote may raises SIGSEGV
  this->session_->on_disconnect.Connect(on_disconnect);
  this->channel_ = zen::remote::server::CreateChannel(this->session_);
}
Session::~Session() {
  LOG_DEBUG("(Session destructor, peer id=%lu)", peer_id_);
}
[[nodiscard]] uint64_t Session::id() const {
  return this->peer_id_;
}
[[nodiscard]] std::shared_ptr<zen::remote::server::IChannel> Session::channel()
    const {
  return this->channel_;
}
}  // namespace yaza::remote
