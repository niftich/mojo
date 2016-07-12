#!/bin/bash
# Copyright 2016 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

set -e

readonly SCRIPT_ROOT="$(cd $(dirname ${BASH_SOURCE[0]}) && pwd)"

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

function download_tool() {
  local name="${1}"
  local tool_path="${SCRIPT_ROOT}/${2}"
  local bucket="${3}"
  local stamp_path="${tool_path}.stamp"
  local requested_hash="$(cat "${tool_path}.sha1")"
  local tool_url="https://storage.googleapis.com/${bucket}/${requested_hash}"

  if [[ ! -f "${stamp_path}" || "${requested_hash}" != "$(cat "${stamp_path}")" ]]; then
    echo "Downloading ${name}..."
    rm -f -- "${tool_path}"
    curl --progress-bar -continue-at=- --location --output "${tool_path}" "${tool_url}"
    chmod +x "${tool_path}"
    echo "${requested_hash}" > "${stamp_path}"
  fi
}

download_tool \
  "mojom compiler" \
  "public/tools/bindings/mojom_tool/bin/${HOST_PLATFORM}/mojom" \
  "mojo/mojom_parser/${HOST_PLATFORM}"

download_tool \
  "mojom c generator" \
  "public/tools/bindings/mojom_tool/bin/${HOST_PLATFORM}/generators/c" \
  "mojo/mojom_parser/${HOST_PLATFORM}/generators"

download_tool \
  "mojom deps generator" \
  "public/tools/bindings/mojom_tool/bin/${HOST_PLATFORM}/generators/deps" \
  "mojo/mojom_parser/${HOST_PLATFORM}/generators"
