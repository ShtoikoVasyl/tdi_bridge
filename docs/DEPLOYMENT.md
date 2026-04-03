# Linux Deployment Guide

This document describes a practical install flow for `tdi_reader` on Linux with `systemd`.

The expected starting point is:

- the repository is already cloned
- you run commands from the repository root

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

Automated install from the repository root:

```bash
sudo bash scripts/install_linux.sh
```

What the script does:

- installs build dependencies
- uses your current Linux user as the service user when the repo is under `/home/<user>/...`
- otherwise creates the `tdi_reader` system user if needed
- adds the chosen service user to `dialout`
- builds the binary from the current repository
- creates `/etc/tdi_reader/config.yaml` if it does not already exist
- installs and enables the `systemd` service

Manual build flow:

```bash
cmake -S . -B build-linux
cmake --build build-linux --config Release -j"$(nproc)"
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
bash scripts/update_linux.sh
```

What the update script does:

- runs `git pull --ff-only` in the current repository
- rebuilds the project
- restarts the `systemd` service through `sudo` when needed

Manual update flow:

```bash
git pull --ff-only
cmake -S . -B build-linux
cmake --build build-linux --config Release -j"$(nproc)"
sudo systemctl restart tdi_reader
```

## Why `build-linux` is used

The project may already contain another CMake build directory created from a different absolute source path.

To avoid cache/source mismatches such as:

```text
CMake Error: The source ".../CMakeLists.txt" does not match the source "..."
```

the deployment scripts use a dedicated `build-linux` directory.

## Important note about serial baud rate

The default config uses:

```yaml
serial_baud: 420000
```

The serial layer first tries exact `420000` on Linux through `termios2` and `BOTHER`.

If the adapter, driver, or kernel still rejects it, the app falls back to the nearest supported standard baudrate and logs that downgrade. For CRSF, you should still verify on the target hardware that the exact rate is really accepted.
