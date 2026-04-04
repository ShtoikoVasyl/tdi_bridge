#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${PROJECT_ROOT}/build-linux}"
SERVICE_NAME="${SERVICE_NAME:-tdi_reader}"

if [[ ! -f "${PROJECT_ROOT}/CMakeLists.txt" ]]; then
  echo "Project root not found at ${PROJECT_ROOT}"
  exit 1
fi

cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}"
cmake --build "${BUILD_DIR}" --config Release -j"$(nproc)"

if [[ "$(id -u)" -eq 0 ]]; then
  systemctl restart "${SERVICE_NAME}"
  systemctl --no-pager --full status "${SERVICE_NAME}"
else
  sudo systemctl restart "${SERVICE_NAME}"
  sudo systemctl --no-pager --full status "${SERVICE_NAME}"
fi
