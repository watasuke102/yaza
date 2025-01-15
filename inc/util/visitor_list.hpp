#pragma once

#include <any>
#include <functional>
#include <utility>
#include <variant>
#include <vector>

namespace yaza::util {
class VisitorList {
 public:
  template <typename... Visitors>
  explicit VisitorList(Visitors... visitors) {
    add_visitors(visitors...);
  }

  template <typename... Variant>
  void visit(std::variant<Variant...>& variant) const {
    std::visit(
        [this](auto&& variant) {
          using T = decltype(variant);
          for (const auto& visitor : visitors_) {
            if (visitor.type() == typeid(std::function<void(T)>)) {
              std::any_cast<const std::function<void(T)>&>(visitor)(variant);
              return;
            }
          }
        },
        variant);
  }
  template <typename... Variant>
  void visit(std::variant<Variant...>&& variant) const {
    std::visit(
        [this](auto&& variant) {
          using T = decltype(variant);
          for (const auto& visitor : visitors_) {
            if (visitor.type() == typeid(std::function<void(T&&)>)) {
              std::any_cast<const std::function<void(T&&)>&>(visitor)(
                  std::forward(variant));
              return;
            }
          }
        },
        std::move(variant));
  }

 private:
  std::vector<std::any> visitors_;

  template <typename Visitor, typename... Rest>
  void add_visitors(Visitor visitor, Rest... rest) {
    visitors_.push_back(std::function(visitor));
    add_visitors(rest...);
  }
  void add_visitors() {
  }
};
}  // namespace yaza::util
