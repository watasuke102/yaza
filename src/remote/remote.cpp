#include "remote/remote.hpp"

#include <wayland-server-core.h>
#include <wayland-util.h>
#include <zen-remote/logger.h>
#include <zen-remote/server/peer-manager.h>

#include <cassert>
#include <cstddef>
#include <exception>
#include <memory>
#include <optional>

#include "common.hpp"
#include "remote/loop.hpp"
#include "util/signal.hpp"

namespace yaza::remote {
namespace {
class LogSink : public zen::remote::ILogSink {
  void Sink(zen::remote::Severity severity, const char* /*pretty_function*/,
      const char* file, int line, const char* format, va_list vp) override {
    using zen::remote::Severity;
    log::Severity s = log::Severity::INFO;
    switch (severity) {
      case Severity::DEBUG:
        s = log::Severity::DEBUG;
        break;
      case Severity::INFO:
        s = log::Severity::INFO;
        break;
      case Severity::WARN:
        s = log::Severity::WARN;
        break;
      case Severity::ERROR:
      case Severity::FATAL:
        s = log::Severity::ERR;
        break;
      default:
        break;
    }
    log::vprintf("ZenRemote", s, file, line, format, vp);
  }
};

}  // namespace
std::unique_ptr<Remote> g_remote = nullptr;  // NOLINT

Remote::Remote(wl_event_loop* loop)
    : wl_loop_(loop)
    , current_session_(std::nullopt)
    , peer_manager_(zen::remote::server::CreatePeerManager(
          std::make_unique<Loop>(loop))) {
  this->peer_manager_->on_peer_discover.Connect([this](uint64_t peer_id) {
    auto peer = this->peer_manager_->Get(peer_id);
    if (!peer) {
      return;
    }
    LOG_DEBUG("(PeerManager) peer is discovered: id=%lu, host=`%s`", peer_id,
        peer->host().c_str());
    if (this->current_session_.has_value()) {
      return;
    }
    try {
      this->current_session_ =
          std::make_unique<Session>(std::move(peer), this->wl_loop_, [this]() {
            LOG_DEBUG("disconnection handler for ISession");
            if (this->current_session_.has_value()) {
              this->disconnect();
            }
          });
    } catch (std::exception& _) {
      this->current_session_ = std::nullopt;
      return;
    }
    LOG_DEBUG("session is established with peer id=%lu", peer_id);
    this->events_.session_established_.emit(this->current_session_->get());
  });

  this->peer_manager_->on_peer_lost.Connect([this](uint64_t peer_id) {
    LOG_DEBUG("(PeerManager) peer is lost      : id=%lu", peer_id);
    if (!this->current_session_.has_value()) {
      return;
    }
  });
}
Remote::~Remote() {
  LOG_DEBUG("destroying Remote");
  if (this->current_session_.has_value()) {
    this->disconnect();
  }
}
std::shared_ptr<zen::remote::server::IChannel> Remote::channel_nonnull() {
  assert(this->current_session_.has_value());
  return this->current_session_->get()->channel();
}
void Remote::listen_session_established(util::Listener<Session*>& listener) {
  this->events_.session_established_.add_listener(listener);
}
void Remote::listen_session_disconnected(
    util::Listener<std::nullptr_t*>& listener) {
  this->events_.session_disconnected_.add_listener(listener);
}
void Remote::disconnect() {
  assert(this->current_session_.has_value());
  LOG_DEBUG(
      "disconnecting session (id=%lu)", this->current_session_->get()->id());
  this->current_session_ = std::nullopt;
  this->events_.session_disconnected_.emit(nullptr);
}

void terminate() {
  g_remote = nullptr;
}
void init(wl_event_loop* loop) {
  zen::remote::InitializeLogger(std::make_unique<LogSink>());
  g_remote = std::make_unique<Remote>(loop);
}
}  // namespace yaza::remote
