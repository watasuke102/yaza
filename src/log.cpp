#include <cstdarg>
#include <cstdio>

#include "common.hpp"

namespace yaza::log {
namespace {
const char* severity_to_str(Severity s) {
  switch (s) {
    case Severity::ERR:
      return "error";
    case Severity::WARN:
      return "warn ";
    case Severity::INFO:
      return "info ";
    case Severity::DEBUG:
      return "debug";
  }
}
}  // namespace

void vprintf(const char* tag, Severity s, const char* file, int line,
    const char* __restrict format, va_list arg) {
  std::printf("[%s:%s|%18s#%04d] ", tag, severity_to_str(s), file, line);
  std::vprintf(format, arg);
  (void)std::putchar('\n');
}
// NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay,
// cert-dcl50-cpp)
void printf(const char* tag, Severity s, const char* file, int line,
    const char* __restrict format, ...) {
  va_list args;
  va_start(args, format);
  log::vprintf(tag, s, file, line, format, args);
  va_end(args);
}
// NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay,
// cert-dcl50-cpp)
}  // namespace yaza::log
