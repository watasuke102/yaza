#include "util/time.hpp"

#include <cstdint>
#include <ctime>

namespace yaza::util {
int64_t now_msec() {
  std::timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
    return -1;
  }
  return (static_cast<int64_t>(ts.tv_sec) * 1000) + (ts.tv_nsec / 1'000'000);
}

}  // namespace yaza::util
