# Build environment for Distributed Libraries
# Uses the Common Utilities, see: https://github.com/massenz/common-utils
#
# Created MM, 2021-11-27

set -eu

BUILDDIR=$(abspath "./build")
CLANG=$(which clang++)

OS_NAME=$(uname -s)
msg "Build Platform: ${OS_NAME}"
if [[ ${OS_NAME} == "Linux" ]]; then
    export LD_LIBRARY_PATH="${BUILDDIR}/lib":${LD_LIBRARY_PATH:-}
elif [[ ${OS_NAME} == "Darwin" ]]; then
  export DYLD_LIBRARY_PATH="${BUILDDIR}/lib":${DYLD_LIBRARY_PATH:-}
else
    fatal "Unsupported Linux variant: ${OS_NAME}"
fi
