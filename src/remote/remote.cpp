#include "remote/remote.hpp"

#include <wayland-server-core.h>
#include <wayland-util.h>
#include <zen-remote/logger.h>
#include <zen-remote/server/peer-manager.h>

#include <cassert>
#include <chrono>
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

constexpr uint64_t kRefreshIntervalNsec = 1'000'000'000.F / 60.F /*FPS*/;
constexpr int      kNsecPerMsec         = 1'000'000;
bool               logger_initialized   = false;
}  // namespace
Remote::Remote(wl_event_loop* loop)
    : wl_loop_(loop)
    , current_session_(std::nullopt)
    , peer_manager_(zen::remote::server::CreatePeerManager(
          std::make_unique<Loop>(loop))) {
  if (!logger_initialized) {
    zen::remote::InitializeLogger(std::make_unique<LogSink>());
    logger_initialized = true;
  }

  this->peer_discover_signal_disconnector_ =
      this->peer_manager_->on_peer_discover.Connect([this](uint64_t peer_id) {
        auto peer = this->peer_manager_->Get(peer_id);
        if (!peer) {
          return;
        }
        LOG_DEBUG("(PeerManager) peer is discovered: id=%lu, host=`%s`",
            peer_id, peer->host().c_str());
        if (this->has_session()) {
          return;
        }
        try {
          this->current_session_ = std::make_unique<Session>(
              std::move(peer), this->wl_loop_, [this]() {
                LOG_DEBUG("disconnection handler for ISession");
                if (this->has_session()) {
                  this->disconnect();
                }
              });
        } catch (std::exception& _) {
          this->current_session_ = std::nullopt;
          return;
        }
        LOG_DEBUG("session is established with peer id=%lu", peer_id);
        this->events_.session_established.emit(this->current_session_->get());
      });

  this->peer_lost_signal_disconnector_ =
      this->peer_manager_->on_peer_lost.Connect([](uint64_t peer_id) {
        LOG_DEBUG("(PeerManager) peer is lost      : id=%lu", peer_id);
      });

  constexpr uint32_t kBusynessThreshold = 100;
  this->frame_timer_source_             = wl_event_loop_add_timer(
      loop,
      [](void* data) {
        auto* self = static_cast<Remote*>(data);

        if (!self->has_session() ||
            self->channel_nonnull()->GetBusyness() < kBusynessThreshold) {
          self->events_.session_frame.emit(nullptr);
        }

        auto now  = std::chrono::steady_clock::now();
        auto next = self->prev_frame_;
        do {
          next += std::chrono::nanoseconds(kRefreshIntervalNsec);
        } while (now > next);
        auto duration_nsec = next - now;
        int  duration_msec =
            static_cast<int>(duration_nsec.count()) / kNsecPerMsec;
        if (duration_msec <= 0) {
          duration_msec = 1;
        }

        wl_event_source_timer_update(self->frame_timer_source_, duration_msec);
        self->prev_frame_ = next;
        return 0;
      },
      this);
  wl_event_source_timer_update(
      this->frame_timer_source_, (kRefreshIntervalNsec / kNsecPerMsec) + 1);
  this->prev_frame_ = std::chrono::steady_clock::now();
}
Remote::~Remote() {
  LOG_DEBUG("destroying Remote");
  this->peer_discover_signal_disconnector_->Disconnect();
  this->peer_lost_signal_disconnector_->Disconnect();
  if (this->has_session()) {
    this->disconnect();
  }
  wl_event_source_remove(this->frame_timer_source_);
}
bool Remote::has_session() {
  return this->current_session_.has_value();
}
std::shared_ptr<zen::remote::server::IChannel> Remote::channel_nonnull() {
  assert(this->current_session_.has_value());
  return this->current_session_->get()->channel();
}
void Remote::listen_session_established(util::Listener<Session*>& listener) {
  this->events_.session_established.add_listener(listener);
}
void Remote::listen_session_disconnected(
    util::Listener<std::nullptr_t*>& listener) {
  this->events_.session_disconnected.add_listener(listener);
}
void Remote::listen_session_frame(util::Listener<std::nullptr_t*>& listener) {
  this->events_.session_frame.add_listener(listener);
}
void Remote::disconnect() {
  assert(this->has_session());
  LOG_DEBUG(
      "disconnecting session (id=%lu)", this->current_session_->get()->id());
  this->current_session_ = std::nullopt;
  this->events_.session_disconnected.emit(nullptr);
}
}  // namespace yaza::remote
