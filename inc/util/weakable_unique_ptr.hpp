#pragma once

#include <memory>

namespace yaza::util {
template <class T>
class WeakPtr;

/// std::unique_ptr that is referable by WeakPtr
template <class T>
class UniPtr {
 public:
  template <class... Args>
  explicit UniPtr(Args&&... args)
      : ptr_(std::make_shared<T>(std::forward<Args>(args)...)) {
  }
  explicit UniPtr(T&& t) : ptr_(std::make_shared(std::move(t))) {
  }
  UniPtr(UniPtr&& other) noexcept            = default;
  UniPtr& operator=(UniPtr&& other) noexcept = default;
  UniPtr(const UniPtr&)                      = delete;
  UniPtr& operator=(const UniPtr&)           = delete;
  ~UniPtr()                                  = default;

  WeakPtr<T> weak() const {
    return WeakPtr(this->ptr_);
  }

  T* get() const {
    return this->ptr_.get();
  }
  T* operator->() const {
    return this->ptr_.get();
  }

 private:
  std::shared_ptr<T> ptr_;
};

template <class T>
class WeakPtr {
 public:
  WeakPtr() = default;
  explicit WeakPtr(std::shared_ptr<T> shared) : ptr_(shared) {
  }
  WeakPtr(WeakPtr&& other) noexcept            = default;
  WeakPtr& operator=(WeakPtr&& other) noexcept = default;
  WeakPtr(const WeakPtr&)                      = default;
  WeakPtr& operator=(const WeakPtr&)           = default;
  ~WeakPtr()                                   = default;

  T* lock() const {
    if (auto p = this->ptr_.lock()) {
      return p.get();
    }
    return nullptr;
  }
  void reset() {
    this->ptr_.reset();
  }
  void swap(WeakPtr<T>& other) noexcept {
    this->ptr_.swap(other.ptr_);
  }

  bool operator==(const WeakPtr<T>& other) {
    return this->lock() == other.lock();
  }
  bool operator!=(const WeakPtr<T>& other) {
    return !(this == other);
  }

 private:
  std::weak_ptr<T> ptr_;
};
}  // namespace yaza::util
