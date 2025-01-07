#pragma once
#include <cstdio>

#define LOG_ERR(fmt, ...)                                                      \
  std::printf("[yaza:error|%15s#%04d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)

#define LOG_WARN(fmt, ...)                                                     \
  std::printf("[yaza:warn |%15s#%04d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)

#define LOG_INFO(fmt, ...)                                                     \
  std::printf("[yaza:info |%15s#%04d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)

#define LOG_DEBUG(fmt, ...)                                                    \
  std::printf("[yaza:debug|%15s#%04d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
