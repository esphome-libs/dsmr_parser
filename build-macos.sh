#!/usr/bin/env bash

set -o xtrace -o errexit -o nounset -o pipefail

readonly currentScriptDir=`cd "$(dirname "${BASH_SOURCE[0]}")" && pwd`
readonly buildDir="${currentScriptDir}/build"
readonly hostArch="$(uname -m)"

build_and_test() {
  local build_type="$1" # Debug or Release
  local target="$2" # macos-arm64 or macos-x86_64

  echo -e "\n\nBuild and test ${target}-${build_type}"

  cmake -S "$currentScriptDir" \
        -B "$buildDir/${target}-${build_type}" \
        -G "Ninja" \
        -D CMAKE_BUILD_TYPE="$build_type" \
        -D CMAKE_OSX_ARCHITECTURES="$hostArch"
  cmake --build "$buildDir/${target}-${build_type}"
  ctest --test-dir "$buildDir/${target}-$build_type"
}

case "$hostArch" in
  arm64)  readonly targetName="macos-arm64" ;;
  x86_64) readonly targetName="macos-x86_64" ;;
  *) echo "Unsupported macOS architecture: $hostArch" >&2; exit 1 ;;
esac

build_and_test Debug "$targetName"
build_and_test Release "$targetName"

echo "Success"
