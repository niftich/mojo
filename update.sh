#!/bin/bash
# Copyright 2016 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -e

readonly SCRIPT_ROOT="$(cd $(dirname ${BASH_SOURCE[0]} ) && pwd)"

case "$(uname -s)" in
  Darwin)
    # //buildtools uses mac, but this tool spells it mac64.
    readonly HOST_PLATFORM="mac64"
    ;;
  Linux)
    readonly HOST_PLATFORM="linux64"
    ;;
  *)
    echo "Unknown operating system. Cannot install build tools."
    exit 1
    ;;
esac

readonly MOJOM_PATH="${SCRIPT_ROOT}/public/tools/bindings/mojom_tool/bin/${HOST_PLATFORM}/mojom"
readonly MOJOM_STAMP_PATH="${MOJOM_PATH}.stamp"
readonly MOJOM_HASH="$(cat "${MOJOM_PATH}.sha1")"
readonly MOJOM_BUCKET="mojo/mojom_parser/${HOST_PLATFORM}"
readonly MOJOM_URL="https://storage.googleapis.com/${MOJOM_BUCKET}/${MOJOM_HASH}"

if [[ ! -f "${MOJOM_STAMP_PATH}" ]] || [[ "${MOJOM_HASH}" != "$(cat "${MOJOM_STAMP_PATH}")" ]]; then
  echo "Downloading mojom..."
  rm -f -- "${MOJOM_PATH}"
  curl --progress-bar -continue-at=- --location --output "${MOJOM_PATH}" "${MOJOM_URL}"
  chmod a+x "${MOJOM_PATH}"
  echo "${MOJOM_HASH}" > "${MOJOM_STAMP_PATH}"
fi
