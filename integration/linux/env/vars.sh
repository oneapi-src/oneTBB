#!/bin/sh
#
# Copyright (c) 2005-2020 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# The script is setting up environment for TBB.
# Supported arguments:
#   intel64|ia32 - architecture, intel64 is default.

if [ -n "$ZSH_EVAL_CONTEXT" ]; then
    case "$0" in
    (/*) TBBROOT="$(dirname $0)"/.. ;;
    (*) TBBROOT="$(dirname "$(command pwd)/$0")"/.. ;;
    esac
elif [ -n "$KSH_VERSION" ]; then
    TBBROOT="${.sh.file%/*}/.."
else
    TBBROOT="$(cd "$(dirname "${BASH_SOURCE}")" && pwd -P)"/..
fi
TBB_TARGET_ARCH="intel64"

for i in "$@"
do
case $i in
    intel64|ia32)
        TBB_TARGET_ARCH="${i}";;
    *) ;;
esac
done

TBB_LIB_NAME="libtbb.so.12"
TBB_LIB_DIR="$TBB_TARGET_ARCH/gcc4.8"

if [ -e "$TBBROOT/lib/$TBB_LIB_DIR/$TBB_LIB_NAME" ]; then
    export TBBROOT
    if [ -z "${LD_LIBRARY_PATH}" ]; then
        LD_LIBRARY_PATH="$TBBROOT/lib/$TBB_LIB_DIR"; export LD_LIBRARY_PATH
    else
        LD_LIBRARY_PATH="$TBBROOT/lib/$TBB_LIB_DIR:${LD_LIBRARY_PATH}"; export LD_LIBRARY_PATH
    fi
    if [ -z "${LIBRARY_PATH}" ]; then
        LIBRARY_PATH="$TBBROOT/lib/$TBB_LIB_DIR"; export LIBRARY_PATH
    else
        LIBRARY_PATH="$TBBROOT/lib/$TBB_LIB_DIR:${LIBRARY_PATH}"; export LIBRARY_PATH
    fi
    if [ -z "${CPATH}" ]; then
        CPATH="${TBBROOT}/include"; export CPATH
    else
        CPATH="${TBBROOT}/include:$CPATH"; export CPATH
    fi

    CMAKE_PREFIX_PATH="${TBBROOT}:${CMAKE_PREFIX_PATH}"; export CMAKE_PREFIX_PATH
else
    echo "ERROR: $TBB_LIB_NAME library does not exist in $TBBROOT/lib/$TBB_LIB_DIR."
    return 2
fi
