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

- rebuilds from the current local repository state
- rebuilds the project
- restarts the `systemd` service through `sudo` when needed

Manual update flow:

```bash
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

## Useful commands

Show the installed systemd unit:

```bash
systemctl cat tdi_reader
```

Show detailed service status:

```bash
sudo systemctl status tdi_reader --no-pager -l
```

Show recent service logs:

```bash
sudo journalctl -u tdi_reader -n 100 --no-pager
```

Watch service logs live:

```bash
sudo journalctl -u tdi_reader -f
```

Show the actual runtime config:

```bash
sudo cat /etc/tdi_reader/config.yaml
```

Check whether the app listens on port `9000`:

```bash
ss -ltnup | grep 9000
```

Check TCP listener only:

```bash
ss -ltn | grep 9000
```

Check UDP listener only:

```bash
ss -lun | grep 9000
```

Show FTDI serial devices:

```bash
ls /dev/ttyUSB*
ls -l /dev/serial/by-id
```

Show USB devices:

```bash
lsusb
```

Check the FTDI kernel driver:

```bash
lsmod | grep ftdi_sio
```

Check device access permissions:

```bash
id
groups
ls -l /dev/ttyUSB0
```

Check path permissions for systemd:

```bash
namei -l /home/$USER/Desktop/tdi_bridge
```

Check firewall rules:

```bash
sudo ufw status
sudo iptables -L -n
```

## Important note about serial baud rate

The default config uses:

```yaml
serial_baud: 420000
```

The serial layer first tries exact `420000` on Linux through `termios2` and `BOTHER`.

If the adapter, driver, or kernel still rejects it, the app falls back to the nearest supported standard baudrate and logs that downgrade. For CRSF, you should still verify on the target hardware that the exact rate is really accepted.
