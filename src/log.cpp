#include <sys/ioctl.h>
#include <unistd.h>

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "common.hpp"

namespace yaza::log {
namespace {
struct LogFormat {
  const char* severity;
  const char* color;
};
// NOLINTBEGIN
constexpr LogFormat kLogFormats[] = {
    [(uint8_t)Severity::INFO]  = {.severity = "info ", .color = "\x1b[7;32m"},
    [(uint8_t)Severity::WARN]  = {.severity = "warn ", .color = "\x1b[7;33m"},
    [(uint8_t)Severity::ERR]   = {.severity = "error", .color = "\x1b[7;31m"},
    [(uint8_t)Severity::DEBUG] = {.severity = "debug", .color = "\x1b[2m"   },
};
constexpr uint8_t kSeverityStrLen = 5;
// clang-format off
static_assert(kLogFormats[(uint8_t)Severity::INFO ].severity[kSeverityStrLen] == '\0');
static_assert(kLogFormats[(uint8_t)Severity::WARN ].severity[kSeverityStrLen] == '\0');
static_assert(kLogFormats[(uint8_t)Severity::ERR  ].severity[kSeverityStrLen] == '\0');
static_assert(kLogFormats[(uint8_t)Severity::DEBUG].severity[kSeverityStrLen] == '\0');
// clang-format on
// NOLINTEND
}  // namespace

void vprintf(const char* tag, Severity s, const char* file, int line,
    const char* __restrict format, va_list arg) {
  const auto& fmt_info = kLogFormats[static_cast<uint8_t>(s)];  // NOLINT
  (void)std::putchar('\r');
#ifndef NO_COLORED_LOG
  (void)std::fputs(fmt_info.color, stdout);
#endif
  std::printf("[%s:%s|%18s#%04d]", tag, fmt_info.severity, file, line);
#ifndef NO_COLORED_LOG
  (void)std::fputs("\x1b[0m ", stdout);
#endif
  std::vprintf(format, arg);
  (void)std::putchar('\n');
}
// NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay,cert-dcl50-cpp)
void printf(const char* tag, Severity s, const char* file, int line,
    const char* __restrict format, ...) {
  va_list args;
  va_start(args, format);
  log::vprintf(tag, s, file, line, format, args);
  va_end(args);
}
// NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay,cert-dcl50-cpp)
}  // namespace yaza::log
