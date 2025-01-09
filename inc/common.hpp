#pragma once
#include <cstdio>
#include <cstdlib>

#define DISABLE_MOVE_AND_COPY(Class)                                           \
  Class(const Class&)            = delete;                                     \
  Class(Class&&)                 = delete;                                     \
  Class& operator=(const Class&) = delete;                                     \
  Class& operator=(Class&&)      = delete

#define LOG_ERR(fmt, ...)                                                      \
  std::printf(                                                                 \
      "[yaza:error|%15s#%04d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define LOG_WARN(fmt, ...)                                                     \
  std::printf(                                                                 \
      "[yaza:warn |%15s#%04d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define LOG_INFO(fmt, ...)                                                     \
  std::printf(                                                                 \
      "[yaza:info |%15s#%04d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define LOG_DEBUG(fmt, ...)                                                    \
  std::printf(                                                                 \
      "[yaza:debug|%15s#%04d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

template <typename T>
inline T zalloc(size_t size) {
  return static_cast<T>(calloc(1, size));
}
