#!/usr/bin/env sh

if [[ -z "$ANDROID_NDK" ]]; then
  echo '[FATAL] $ANDROID_NDK is not set'
  exit 1
fi

set -v

cmake \
  -DCMAKE_SYSTEM_NAME=Android \
  -DCMAKE_SYSTEM_VERSION=31 \
  -DANDROID_PLATFORM=android-31 \
  -DANDROID_ABI=arm64-v8a \
  -DCMAKE_ANDROID_ARCH_ABI=arm64-v8a \
  -DANDROID_NDK=$ANDROID_NDK \
  -DCMAKE_ANDROID_NDK=$ANDROID_NDK \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -Bbuild-android \
  -GNinja \
  .
