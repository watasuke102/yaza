#pragma once
#include <cstdio>
#include <cstdlib>

#define DISABLE_MOVE_AND_COPY(Class)                                           \
  Class(const Class&)            = delete;                                     \
  Class(Class&&)                 = delete;                                     \
  Class& operator=(const Class&) = delete;                                     \
  Class& operator=(Class&&)      = delete

#ifdef __GNUC__
#define ATTRIB_PRINTF(start, end) __attribute__((format(printf, start, end)))
#else
#define ATTRIB_PRINTF(start, end)
#endif
namespace yaza::log {
enum class Severity { ERR, WARN, INFO, DEBUG };
void vprintf(const char* tag, Severity s, const char* file, int line,
    const char* __restrict format, va_list arg) ATTRIB_PRINTF(5, 0);
void printf(const char* tag, Severity s, const char* file, int line,
    const char* __restrict format, ...) ATTRIB_PRINTF(5, 6);
}  // namespace yaza::log
#undef ATTRIB_PRINTF

#define LOG_ERR(fmt, ...)                                                      \
  yaza::log::printf("yaza", yaza::log::Severity::ERR, __FILE__, __LINE__, fmt, \
      ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)                                                     \
  yaza::log::printf("yaza", yaza::log::Severity::WARN, __FILE__, __LINE__,     \
      fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)                                                     \
  yaza::log::printf("yaza", yaza::log::Severity::INFO, __FILE__, __LINE__,     \
      fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...)                                                    \
  yaza::log::printf("yaza", yaza::log::Severity::DEBUG, __FILE__, __LINE__,    \
      fmt, ##__VA_ARGS__)

template <typename T>
inline T zalloc(size_t size) {
  return static_cast<T>(calloc(1, size));
}
