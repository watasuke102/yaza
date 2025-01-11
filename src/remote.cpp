
#include "remote.hpp"

#include <wayland-server-core.h>
#include <wayland-util.h>
#include <zen-remote/logger.h>
#include <zen-remote/loop.h>
#include <zen-remote/server/peer-manager.h>

#include <cassert>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <optional>

#include "common.hpp"
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

class Loop : public zen::remote::ILoop {
 public:
  DISABLE_MOVE_AND_COPY(Loop);
  explicit Loop(wl_event_loop* wl_loop) : wl_loop_(wl_loop) {};
  ~Loop() override = default;
  void AddFd(zen::remote::FdSource* source) override {
    using zen::remote::FdSource;
    uint32_t mask = 0;
    if (source->mask & FdSource::kReadable) {
      mask |= WL_EVENT_READABLE;
    }
    if (source->mask & FdSource::kWritable) {
      mask |= WL_EVENT_WRITABLE;
    }
    if (source->mask & FdSource::kHangup) {
      mask |= WL_EVENT_HANGUP;
    }
    if (source->mask & FdSource::kError) {
      mask |= WL_EVENT_ERROR;
    }

    auto handle_loop_callback = [](int fd, uint32_t mask, void* data) {
      uint32_t remote_mask = 0;
      if (mask & WL_EVENT_READABLE) {
        remote_mask |= FdSource::kReadable;
      }
      if (mask & WL_EVENT_WRITABLE) {
        remote_mask |= FdSource::kWritable;
      }
      if (mask & WL_EVENT_HANGUP) {
        remote_mask |= FdSource::kHangup;
      }
      if (mask & WL_EVENT_ERROR) {
        remote_mask |= FdSource::kError;
      }
      static_cast<zen::remote::FdSource*>(data)->callback(fd, remote_mask);
      return 1;
    };
    source->data = wl_event_loop_add_fd(
        wl_loop_, source->fd, mask, handle_loop_callback, source);
  }
  void RemoveFd(zen::remote::FdSource* source) override {
    wl_event_source_remove(static_cast<wl_event_source*>(source->data));
  }
  void Terminate() override {
    // TODO
  }

 private:
  wl_event_loop* wl_loop_;
};

}  // namespace
std::unique_ptr<Remote> g_remote = nullptr;  // NOLINT

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
  if (this->current_session_.has_value()) {
    this->disconnect();
  }
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

void create(wl_event_loop* loop) {
  zen::remote::InitializeLogger(std::make_unique<LogSink>());
  g_remote = std::make_unique<Remote>(loop);
}
}  // namespace yaza::remote
