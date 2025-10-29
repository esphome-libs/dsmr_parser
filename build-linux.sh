#!/usr/bin/env bash
# This script requires clang to be installed:
#   sudo apt-get update && sudo apt-get install -y clang

set -o xtrace -o errexit -o nounset -o pipefail
readonly currentScriptDir=`dirname "$(realpath -s "${BASH_SOURCE[0]}")"`
readonly buildDir="${currentScriptDir}/build"

build_and_test() {
  local build_type="$1" # Debug or Release
  local target="$2" # linux-gcc or linux-clang

  echo -e "\n\nBuild and test ${target}-$build_type"

  cmake -S "$currentScriptDir" \
        -B "$buildDir/${target}-$build_type" \
        -G "Ninja" \
        -D CMAKE_BUILD_TYPE="$build_type"
  cmake --build "$buildDir/${target}-$build_type"
  "$buildDir/${target}-$build_type/arduino_dsmr_test"
}

build_and_test Debug linux-gcc
build_and_test Release linux-gcc

export CC=clang
export CXX=clang++
build_and_test Debug linux-clang
build_and_test Release linux-clang

echo "Success"
