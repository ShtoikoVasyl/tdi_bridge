# Linux Deployment Guide

This document describes a practical server-side install flow for `tdi_reader` on Linux with `systemd`.

## 1. Prepare the Linux environment

Required packages on Ubuntu/Debian:

```bash
sudo apt update
sudo apt install -y build-essential cmake git pkg-config udev
```

Required system capabilities:

- Linux with `/dev`, `/sys`, `termios`, and `systemd`
- FTDI kernel drivers: `usbserial` and `ftdi_sio`
- access to the serial adapter as `/dev/ttyUSB*` or `/dev/serial/by-id/*`
- firewall allowance for port `9000` over the chosen transport

Recommended checks:

```bash
lsmod | grep ftdi_sio
ls -l /dev/serial/by-id
id
groups
```

If the service user must access the device, it should be in the `dialout` group.

## 2. Install and build from the repository

Automated install:

```bash
sudo bash scripts/install_linux.sh <repo-url> [branch]
```

Example:

```bash
sudo bash scripts/install_linux.sh https://github.com/your-org/tdi_reader.git main
```

What the script does:

- installs build dependencies
- creates the `tdi_reader` system user if needed
- adds that user to `dialout`
- clones or updates the repository into `/opt/tdi_reader/repo`
- builds the binary with CMake
- creates `/etc/tdi_reader/config.yaml` if it does not already exist
- installs and enables the `systemd` service

Manual build flow:

```bash
git clone <repo-url>
cd tdi_reader
cmake -S . -B build
cmake --build build --config Release -j"$(nproc)"
```

## 3. Run, stop, restart

After installation, service management is done via `systemd`.

Start:

```bash
sudo systemctl start tdi_reader
```

Stop:

```bash
sudo systemctl stop tdi_reader
```

Restart:

```bash
sudo systemctl restart tdi_reader
```

Enable on boot:

```bash
sudo systemctl enable tdi_reader
```

Check status:

```bash
sudo systemctl status tdi_reader
```

Read logs:

```bash
sudo journalctl -u tdi_reader -f
```

## 4. Update, rebuild, restart

Automated update:

```bash
sudo bash scripts/update_linux.sh
```

If you deploy from a branch other than `main`:

```bash
sudo BRANCH=develop bash scripts/update_linux.sh
```

What the update script does:

- fetches the latest code
- checks out the target branch
- pulls the latest commits
- rebuilds the project
- restarts the `systemd` service

Manual update flow:

```bash
cd /opt/tdi_reader/repo
git fetch --all --tags
git checkout main
git pull --ff-only origin main
cmake -S . -B build
cmake --build build --config Release -j"$(nproc)"
sudo systemctl restart tdi_reader
```

## Important note about serial baud rate

The default config uses:

```yaml
serial_baud: 420000
```

The current code uses standard `termios` speed constants. On some Linux systems, `420000` may not work until the project is extended with `termios2` and `BOTHER` support for custom baud rates.

If your adapter or kernel does not accept that speed, use a standard supported baud rate or extend the serial layer before production rollout.
