#!/usr/bin/env bash
set -euo pipefail

REPO_URL="${1:-}"
BRANCH="${2:-main}"
INSTALL_ROOT="${INSTALL_ROOT:-/opt/tdi_reader}"
APP_USER="${APP_USER:-tdi_reader}"
CONFIG_DIR="${CONFIG_DIR:-/etc/tdi_reader}"
SERVICE_NAME="${SERVICE_NAME:-tdi_reader}"

if [[ -z "${REPO_URL}" ]]; then
  echo "Usage: $0 <repo-url> [branch]"
  exit 1
fi

if [[ "$(id -u)" -ne 0 ]]; then
  echo "Run this script as root"
  exit 1
fi

apt-get update
apt-get install -y build-essential cmake git pkg-config udev

if ! id "${APP_USER}" >/dev/null 2>&1; then
  useradd --system --create-home --shell /usr/sbin/nologin "${APP_USER}"
fi

usermod -aG dialout "${APP_USER}"

mkdir -p "${INSTALL_ROOT}" "${CONFIG_DIR}"

if [[ ! -d "${INSTALL_ROOT}/repo/.git" ]]; then
  git clone --branch "${BRANCH}" "${REPO_URL}" "${INSTALL_ROOT}/repo"
else
  git -C "${INSTALL_ROOT}/repo" fetch --all --tags
  git -C "${INSTALL_ROOT}/repo" checkout "${BRANCH}"
  git -C "${INSTALL_ROOT}/repo" pull --ff-only origin "${BRANCH}"
fi

cmake -S "${INSTALL_ROOT}/repo" -B "${INSTALL_ROOT}/repo/build"
cmake --build "${INSTALL_ROOT}/repo/build" --config Release -j"$(nproc)"

ln -sfn "${INSTALL_ROOT}/repo" "${INSTALL_ROOT}/current"

if [[ ! -f "${CONFIG_DIR}/config.yaml" ]]; then
  install -m 0644 "${INSTALL_ROOT}/repo/config/config.yaml" "${CONFIG_DIR}/config.yaml"
fi

install -m 0644 "${INSTALL_ROOT}/repo/deploy/systemd/tdi_reader.service" "/etc/systemd/system/${SERVICE_NAME}.service"

sed -i "s|User=tdi_reader|User=${APP_USER}|g" "/etc/systemd/system/${SERVICE_NAME}.service"
sed -i "s|WorkingDirectory=/opt/tdi_reader/current|WorkingDirectory=${INSTALL_ROOT}/current|g" "/etc/systemd/system/${SERVICE_NAME}.service"
sed -i "s|ExecStart=/opt/tdi_reader/current/build/tdi_reader /etc/tdi_reader/config.yaml|ExecStart=${INSTALL_ROOT}/current/build/tdi_reader ${CONFIG_DIR}/config.yaml|g" "/etc/systemd/system/${SERVICE_NAME}.service"

systemctl daemon-reload
systemctl enable --now "${SERVICE_NAME}"

echo "Installed ${SERVICE_NAME}"
echo "Config: ${CONFIG_DIR}/config.yaml"
echo "Repo: ${INSTALL_ROOT}/repo"
