#!/usr/bin/env bash
set -euo pipefail

INSTALL_ROOT="${INSTALL_ROOT:-/opt/tdi_reader}"
BRANCH="${BRANCH:-main}"
SERVICE_NAME="${SERVICE_NAME:-tdi_reader}"

if [[ "$(id -u)" -ne 0 ]]; then
  echo "Run this script as root"
  exit 1
fi

if [[ ! -d "${INSTALL_ROOT}/repo/.git" ]]; then
  echo "Repository not found at ${INSTALL_ROOT}/repo"
  exit 1
fi

git -C "${INSTALL_ROOT}/repo" fetch --all --tags
git -C "${INSTALL_ROOT}/repo" checkout "${BRANCH}"
git -C "${INSTALL_ROOT}/repo" pull --ff-only origin "${BRANCH}"

cmake -S "${INSTALL_ROOT}/repo" -B "${INSTALL_ROOT}/repo/build"
cmake --build "${INSTALL_ROOT}/repo/build" --config Release -j"$(nproc)"

systemctl restart "${SERVICE_NAME}"
systemctl --no-pager --full status "${SERVICE_NAME}"
