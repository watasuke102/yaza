cmake_minimum_required(VERSION 3.17) # For GIT_SUBMODULES_RECURSE
project(yaza LANGUAGES C CXX)

set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/local.cmake)
  include(${CMAKE_CURRENT_LIST_DIR}/local.cmake)
endif()

set(TARGET_ABI x86_64)
set(ZEN_REMOTE_GRPC_SYSROOT ${GRPC_SYSROOT_NATIVE})
if (CMAKE_ANDROID_ARCH_ABI)
  set(TARGET_ABI ${CMAKE_ANDROID_ARCH_ABI})
  set(ZEN_REMOTE_GRPC_SYSROOT ${GRPC_SYSROOT_ANDROID})
endif()
set(EXTERNAL_PROJ_PREFIX ${CMAKE_CURRENT_LIST_DIR}/3rdparty/root-${TARGET_ABI})
list(APPEND CMAKE_FIND_ROOT_PATH ${EXTERNAL_PROJ_PREFIX})
if (CMAKE_ANDROID_NDK)
  configure_file(${CMAKE_CURRENT_LIST_DIR}/meson_cross.txt.in meson_cross.txt)
  set(MESON_ADDITIONAL_ARG --cross-file ${CMAKE_BINARY_DIR}/meson_cross.txt)
  set(CONFIGURE_ADDITIONAL_ARG --host=aarch64-unknown-linux-android CC=${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android31-clang CXX=${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android31-clang++ LD=${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin/ld "CPP=${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android31-clang -E" "CXXCPP=${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android31-clang++ -E")
endif()

# Use FetchContent (not ExternalProject) in order to re-use CMake toolchain configrations
set(ZEN_REMOTE_REQUIRED_VERSION 0.1.2)
set(ZEN_REMOTE_GRAPHICS_API GLESv3)
set(ZEN_REMOTE_SERVER ON)
set(ZEN_REMOTE_CLIENT OFF)
set(ZEN_REMOTE_PROTOC_EXECUTABLE          ${GRPC_SYSROOT_NATIVE}/bin/protoc)
set(ZEN_REMOTE_GRPC_CPP_PLUGIN_EXECUTABLE ${GRPC_SYSROOT_NATIVE}/bin/grpc_cpp_plugin)
set(CMAKE_CXX_FLAGS "-DBOOST_NO_CXX98_FUNCTION_BASE")
include(FetchContent)
FetchContent_Declare(zen-remote
  GIT_REPOSITORY https://github.com/watasuke102/zen-remote GIT_TAG main GIT_SHALLOW true GIT_SUBMODULES_RECURSE true
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/3rdparty/zen-remote
  PATCH_COMMAND sed -i "s|SHARED|STATIC|" <SOURCE_DIR>/src/server/CMakeLists.txt
)
FetchContent_MakeAvailable(zen-remote)
set(CMAKE_CXX_FLAGS "")

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
ExternalProject_Add(wayland-protocols
  GIT_REPOSITORY https://gitlab.freedesktop.org/wayland/wayland-protocols.git GIT_TAG 1.39 GIT_SHALLOW true
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/3rdparty/wayland-protocols
  INSTALL_DIR ${EXTERNAL_PROJ_PREFIX}
  UPDATE_COMMAND ""
  CONFIGURE_COMMAND cd <SOURCE_DIR> && ${MESON_PROGRAM} setup      build-${TARGET_ABI} ${MESON_ADDITIONAL_ARG} --prefix <INSTALL_DIR> -Dtests=false
  BUILD_COMMAND     cd <SOURCE_DIR> && ${MESON_PROGRAM} compile -C build-${TARGET_ABI}
  INSTALL_COMMAND   cd <SOURCE_DIR> && ${MESON_PROGRAM} install -C build-${TARGET_ABI}
  DEPENDS libffi
)
ExternalProject_Add(zwin-protocols
  GIT_REPOSITORY https://github.com/watasuke102/zwin GIT_TAG main GIT_SHALLOW true
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/3rdparty/zwin-protocols
  INSTALL_DIR ${EXTERNAL_PROJ_PREFIX}
  UPDATE_COMMAND ""
  CONFIGURE_COMMAND cd <SOURCE_DIR> && ${MESON_PROGRAM} setup      build-${TARGET_ABI} ${MESON_ADDITIONAL_ARG} --prefix <INSTALL_DIR> -Dtests=false
  BUILD_COMMAND     cd <SOURCE_DIR> && ${MESON_PROGRAM} compile -C build-${TARGET_ABI}
  INSTALL_COMMAND   cd <SOURCE_DIR> && ${MESON_PROGRAM} install -C build-${TARGET_ABI}
  DEPENDS wayland
)
find_program(WAYLAND_SCANNER_PROGRAM NAMES wayland-scanner REQUIRED)
set(GENERATED_PROTO_DST ${CMAKE_BINARY_DIR}/protocols)

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
  ${CMAKE_CURRENT_LIST_DIR}/inc/*.hpp
)
set(local_inc_dirs
  ${CMAKE_CURRENT_LIST_DIR}/inc
)
set(lib_inc_dirs
  ${EXTERNAL_PROJ_PREFIX}/include
  ${GENERATED_PROTO_DST}
  ${CMAKE_CURRENT_LIST_DIR}/3rdparty/zen-remote/include
)
set(link_libs
  ${EXTERNAL_PROJ_PREFIX}/lib/libwayland-server.a
  ${EXTERNAL_PROJ_PREFIX}/lib/libffi.a
  zen_remote::server
)

function(generate_proto_header file_name xml_relative_path)
  set(source_output ${GENERATED_PROTO_DST}/${file_name}.c)
  set(header_output ${GENERATED_PROTO_DST}/${file_name}.h)
  set(xml_path ${EXTERNAL_PROJ_PREFIX}/share/${xml_relative_path})
  add_custom_command(
    OUTPUT ${source_output}
    COMMAND ${WAYLAND_SCANNER_PROGRAM} private-code ${xml_path} ${source_output}
    DEPENDS wayland-protocols zwin-protocols ${WAYLAND_SCANNER_PROGRAM}
    VERBATIM
  )
  add_custom_command(
    OUTPUT ${header_output}
    COMMAND ${WAYLAND_SCANNER_PROGRAM} server-header ${xml_path} ${header_output}
    DEPENDS wayland-protocols zwin-protocols ${WAYLAND_SCANNER_PROGRAM}
    VERBATIM
  )
  set_source_files_properties(${source_output} GENERATED)
  set_source_files_properties(${header_output} GENERATED)
  set(sources ${sources} ${source_output} PARENT_SCOPE)
  set(headers ${headers} ${header_output} PARENT_SCOPE)
endfunction(generate_proto_header)
generate_proto_header("xdg-shell-protocol" "wayland-protocols/stable/xdg-shell/xdg-shell.xml")
generate_proto_header("zwin-protocol" "zwin/zwin.xml")
generate_proto_header("zwin-shell-protocol" "zwin/zwin-shell.xml")
generate_proto_header("zwin-gles-v32-protocol" "zwin/zwin-gles-v32.xml")

add_executable(${PROJECT_NAME} ${sources} ${headers})
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Wno-gnu-zero-variadic-macro-arguments -ffile-prefix-map=${CMAKE_SOURCE_DIR}/src/=)
target_include_directories(${PROJECT_NAME} PRIVATE ${local_inc_dirs})
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${lib_inc_dirs})
target_link_libraries(${PROJECT_NAME} ${link_libs})

add_dependencies(${PROJECT_NAME} wayland)
