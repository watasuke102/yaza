#pragma once

#include <wayland-server-core.h>
#include <wayland-util.h>

#include <functional>
#include <optional>
#include <type_traits>

#include "common.hpp"

namespace yaza::util {
template <typename EventDataType>
using SignalHandler = std::function<void(EventDataType)>;
// Hereafter, `EventDataType` will be referred to as `D` (event 'D'ata type)

template <typename D>
class Signal;  // forward declaration for using `friend`

template <typename D>
class Listener {
 public:
  static_assert(std::is_pointer_v<D>);

  DISABLE_MOVE_AND_COPY(Listener);
  Listener() {
    child_.parent = this;
    wl_list_init(&this->child_.listener.link);
    this->child_.listener.notify = [](wl_listener* listener, void* data) {
      Child* child = wl_container_of(listener, child, listener);
      if (child->parent->handler_.has_value()) {
        child->parent->handler_.value()(static_cast<D>(data));
      }
    };
  }
  ~Listener() {
    this->remove();
  }

  void set_handler(SignalHandler<D> handler) {
    this->handler_ = handler;
  }
  void remove() {
    wl_list_remove(&this->child_.listener.link);
    wl_list_init(&this->child_.listener.link);
  }

 private:
  /*
    - `wl_container_of` can't use for the class owning SignalHandler
      because std::function is not a standard layout
    - Lambda function that captures variables cannot be assigned to
      `wl_listener::notify` directly
    To handle these problems, `wl_listener` is given to the child struct
    and `wl_container_of` is applied for the child struct.
    See Listener::Listener() for more
   */

  struct Child {
    wl_listener  listener;
    Listener<D>* parent;
  } child_;
  static_assert(std::is_standard_layout_v<Child>);

  std::optional<SignalHandler<D>> handler_ = std::nullopt;
  friend class Signal<D>;
};

template <typename D>
class Signal {
 public:
  static_assert(std::is_pointer_v<D>);

  DISABLE_MOVE_AND_COPY(Signal);
  Signal() {
    wl_signal_init(&this->signal_);
  }
  ~Signal() {
    wl_list_remove(&this->signal_.listener_list);
  }

  void emit(D data) {
    wl_signal_emit(&this->signal_, data);
  }
  void add_listener(Listener<D>& listener) {
    wl_signal_add(&this->signal_, &listener.child_.listener);
  }

 private:
  wl_signal signal_;
};
}  // namespace yaza::util
