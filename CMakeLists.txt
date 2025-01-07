cmake_minimum_required(VERSION 3.10)
project(yaza LANGUAGES CXX)

set(TARGET_ABI x86_64)
if (CMAKE_ANDROID_ARCH_ABI)
  set(TARGET_ABI ${CMAKE_ANDROID_ARCH_ABI})
endif()
set(EXTERNAL_PROJ_PREFIX ${CMAKE_CURRENT_LIST_DIR}/3rdparty/root-${TARGET_ABI})
if (CMAKE_ANDROID_NDK)
  configure_file(${CMAKE_CURRENT_LIST_DIR}/meson_cross.txt.in meson_cross.txt)
  set(MESON_ADDITIONAL_ARG --cross-file ${CMAKE_BINARY_DIR}/meson_cross.txt)
  set(CONFIGURE_ADDITIONAL_ARG --host=aarch64-unknown-linux-android CC=${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android31-clang CXX=${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android31-clang++ LD=${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin/ld "CPP=${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android31-clang -E" "CXXCPP=${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android31-clang++ -E")
endif()

find_program(MESON_PROGRAM NAMES meson REQUIRED)
include(ExternalProject)
ExternalProject_Add(libffi
  GIT_REPOSITORY https://github.com/libffi/libffi.git GIT_TAG v3.4.6 GIT_SHALLOW true
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/3rdparty/libffi
  INSTALL_DIR ${EXTERNAL_PROJ_PREFIX}
  UPDATE_COMMAND ""
  CONFIGURE_COMMAND cd <SOURCE_DIR> && ./autogen.sh && mkdir -p build-${TARGET_ABI} && cd build-${TARGET_ABI} && ../configure --prefix=<INSTALL_DIR> ${CONFIGURE_ADDITIONAL_ARG}
  BUILD_COMMAND     cd <SOURCE_DIR>/build-${TARGET_ABI} && make
  INSTALL_COMMAND   cd <SOURCE_DIR>/build-${TARGET_ABI} && make install
  BUILD_BYPRODUCTS ${EXTERNAL_PROJ_PREFIX}/lib/libffi.a
)
ExternalProject_Add(wayland
  GIT_REPOSITORY https://gitlab.freedesktop.org/wayland/wayland.git GIT_TAG 1.23.1 GIT_SHALLOW true
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/3rdparty/wayland
  INSTALL_DIR ${EXTERNAL_PROJ_PREFIX}
  UPDATE_COMMAND ""
  CONFIGURE_COMMAND cd <SOURCE_DIR> && ${MESON_PROGRAM} setup      build-${TARGET_ABI} ${MESON_ADDITIONAL_ARG} --prefix <INSTALL_DIR> -Ddefault_library=static -Dtests=false -Dscanner=false -Ddocumentation=false -Ddtd_validation=false
  BUILD_COMMAND     cd <SOURCE_DIR> && ${MESON_PROGRAM} compile -C build-${TARGET_ABI}
  INSTALL_COMMAND   cd <SOURCE_DIR> && ${MESON_PROGRAM} install -C build-${TARGET_ABI}
  BUILD_BYPRODUCTS ${EXTERNAL_PROJ_PREFIX}/lib/libwayland-server.a
  DEPENDS libffi
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON) # for CMake >= 3.24
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

file(
  GLOB_RECURSE sources
  ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
)
file(
  GLOB_RECURSE headers
  ${CMAKE_CURRENT_LIST_DIR}/inc/*.h
)
set(inc_dirs
  ${CMAKE_CURRENT_LIST_DIR}/inc
  ${EXTERNAL_PROJ_PREFIX}/include
  ${EXTERNAL_PROJ_PREFIX}/include/pixman-1
  ${PROTO_HEADER_DIR}
)
set(link_libs
  ${EXTERNAL_PROJ_PREFIX}/lib/libwayland-server.a
  ${EXTERNAL_PROJ_PREFIX}/lib/libffi.a
)

add_executable(${PROJECT_NAME} ${sources} ${headers})
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Wno-gnu-zero-variadic-macro-arguments -ffile-prefix-map=${CMAKE_SOURCE_DIR}/src/=)
target_include_directories(${PROJECT_NAME} PUBLIC ${inc_dirs})
target_link_libraries(${PROJECT_NAME} ${link_libs})

add_dependencies(${PROJECT_NAME} wayland)
