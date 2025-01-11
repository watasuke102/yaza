#include "remote/loop.hpp"

#include <wayland-server-core.h>
#include <wayland-util.h>
#include <zen-remote/loop.h>

namespace yaza::remote {
Loop::Loop(wl_event_loop* wl_loop) : wl_loop_(wl_loop) {};
void Loop::AddFd(zen::remote::FdSource* source) {
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
void Loop::RemoveFd(zen::remote::FdSource* source) {
  wl_event_source_remove(static_cast<wl_event_source*>(source->data));
}
void Loop::Terminate() {
  // TODO
}
}  // namespace yaza::remote
