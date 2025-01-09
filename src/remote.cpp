#include "remote.hpp"

#include <zen-remote/logger.h>
#include <zen-remote/loop.h>
#include <zen-remote/server/peer-manager.h>

#include <memory>

#include "common.hpp"

namespace {
class LogSink : public zen::remote::ILogSink {
  void Sink(zen::remote::Severity severity, const char* /*pretty_function*/,
      const char* file, int line, const char* format, va_list vp) override {
    using zen::remote::Severity;
    yaza::log::Severity s = yaza::log::Severity::INFO;
    switch (severity) {
      case Severity::DEBUG:
        s = yaza::log::Severity::DEBUG;
        break;
      case Severity::INFO:
        s = yaza::log::Severity::INFO;
        break;
      case Severity::WARN:
        s = yaza::log::Severity::WARN;
        break;
      case Severity::ERROR:
      case Severity::FATAL:
        s = yaza::log::Severity::ERR;
        break;
      default:
        break;
    }
    yaza::log::vprintf("ZenRemote", s, file, line, format, vp);
  }
};

int loop_callback(int fd, uint32_t mask, void* data) {
  using zen::remote::FdSource;
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
}
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
    source->data =
        wl_event_loop_add_fd(wl_loop_, source->fd, mask, loop_callback, source);
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

class Remote {
 public:
  DISABLE_MOVE_AND_COPY(Remote);
  explicit Remote(std::unique_ptr<zen::remote::ILoop> loop)
      : peer_manager_(zen::remote::server::CreatePeerManager(std::move(loop))) {
    this->peer_manager_->on_peer_discover.Connect([this](uint64_t peer_id) {
      auto peer_data = this->peer_manager_->Get(peer_id);
      if (!peer_data) {
        return;
      }
      LOG_DEBUG(
          "peer is discovered: %lu, %s", peer_id, peer_data->host().c_str());
    });
    this->peer_manager_->on_peer_lost.Connect([](uint64_t peer_id) {
      LOG_DEBUG("peer disconnected : %lu", peer_id);
    });
  }
  ~Remote() = default;

 private:
  std::unique_ptr<zen::remote::server::IPeerManager> peer_manager_;
};
}  // namespace

namespace yaza::remote {
void create(wl_event_loop* loop) {
  zen::remote::InitializeLogger(std::make_unique<LogSink>());
  auto* remote = new Remote(std::make_unique<Loop>(loop));
}
}  // namespace yaza::remote
