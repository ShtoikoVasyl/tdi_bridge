#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${PROJECT_ROOT}/build-linux}"
APP_USER="${APP_USER:-tdi_reader}"
CONFIG_DIR="${CONFIG_DIR:-/etc/tdi_reader}"
SERVICE_NAME="${SERVICE_NAME:-tdi_reader}"

if [[ -n "${SUDO_USER:-}" && "${PROJECT_ROOT}" == "/home/${SUDO_USER}"* && -z "${APP_USER:-}" ]]; then
  APP_USER="${SUDO_USER}"
fi

if [[ -n "${SUDO_USER:-}" && "${PROJECT_ROOT}" == "/home/${SUDO_USER}"* && "${APP_USER}" == "tdi_reader" ]]; then
  APP_USER="${SUDO_USER}"
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

APP_GROUP="$(id -gn "${APP_USER}")"

usermod -aG dialout "${APP_USER}"

mkdir -p "${CONFIG_DIR}"
mkdir -p "${BUILD_DIR}"

chown -R "${APP_USER}:${APP_GROUP}" "${PROJECT_ROOT}"
chown -R "${APP_USER}:${APP_GROUP}" "${BUILD_DIR}"

sudo -u "${APP_USER}" cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}"
sudo -u "${APP_USER}" cmake --build "${BUILD_DIR}" --config Release -j"$(nproc)"

if [[ ! -f "${CONFIG_DIR}/config.yaml" ]]; then
  install -m 0644 "${PROJECT_ROOT}/config/config.yaml" "${CONFIG_DIR}/config.yaml"
fi

install -m 0644 "${PROJECT_ROOT}/deploy/systemd/tdi_reader.service" "/etc/systemd/system/${SERVICE_NAME}.service"

sed -i "s|__APP_USER__|${APP_USER}|g" "/etc/systemd/system/${SERVICE_NAME}.service"
sed -i "s|__APP_GROUP__|${APP_GROUP}|g" "/etc/systemd/system/${SERVICE_NAME}.service"
sed -i "s|__PROJECT_ROOT__|${PROJECT_ROOT}|g" "/etc/systemd/system/${SERVICE_NAME}.service"
sed -i "s|__CONFIG_PATH__|${CONFIG_DIR}/config.yaml|g" "/etc/systemd/system/${SERVICE_NAME}.service"

systemctl daemon-reload
systemctl enable --now "${SERVICE_NAME}"

echo "Installed ${SERVICE_NAME}"
echo "Service user: ${APP_USER}"
echo "Config: ${CONFIG_DIR}/config.yaml"
echo "Project root: ${PROJECT_ROOT}"
echo "Build dir: ${BUILD_DIR}"
