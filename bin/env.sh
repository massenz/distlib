#!/bin/bash

set -eu

source ${COMMON_UTILS_DIR}/utils.sh

BASEDIR="$(abspath $(dirname $0)/..)"
BUILDDIR="${BASEDIR}/build"
CLANG=$(which clang++)

OS_NAME=$(uname -s)
if [[ ${OS_NAME} == "Linux" ]]; then
  export LD_LIBRARY_PATH="${BUILDDIR}/lib":${LD_LIBRARY_PATH:-}
elif [[ ${OS_NAME} == "Darwin" ]]; then
  export DYLD_LIBRARY_PATH="${BUILDDIR}/lib":${DYLD_LIBRARY_PATH:-}
else
    echo "[ERROR] Unsupported Linux variant: ${OS_NAME}"
    exit 1
fi

if [[ -d ${COMMON_UTILS_DIR} ]]; then
  UTILS="-DCOMMON_UTILS_DIR=${COMMON_UTILS_DIR}"
fi

