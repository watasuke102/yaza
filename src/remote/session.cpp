#include "remote/session.hpp"

#include "common.hpp"
#include "remote/loop.hpp"

namespace yaza::remote {
Session::Session(const std::shared_ptr<zen::remote::server::IPeer>& peer,
    wl_event_loop* wl_loop, std::function<void()>&& on_disconnect)
    : wl_loop_(wl_loop)
    , peer_id_(peer->id())
    , session_(zen::remote::server::CreateSession(
          std::make_unique<Loop>(this->wl_loop_))) {
  if (!this->session_->Connect(peer)) {
    LOG_WARN("Failed to connect with peer %lu", peer->id());
    throw std::exception();
  }
  LOG_DEBUG("(Session constructor)");
  this->disconnect_signal_disconnector_ =
      this->session_->on_disconnect.Connect(std::move(on_disconnect));
  this->channel_ = zen::remote::server::CreateChannel(this->session_);
}
Session::~Session() {
  LOG_DEBUG("(Session destructor, peer id=%lu)", peer_id_);
  this->disconnect_signal_disconnector_->Disconnect();
}
[[nodiscard]] uint64_t Session::id() const {
  return this->peer_id_;
}
[[nodiscard]] std::shared_ptr<zen::remote::server::IChannel> Session::channel()
    const {
  return this->channel_;
}
}  // namespace yaza::remote
