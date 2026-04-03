# tdi_reader Ops Quick Start

Run all commands from the repository root.

## 1. Prepare Linux

```bash
sudo apt update
sudo apt install -y build-essential cmake git pkg-config udev
```

## 2. Install service and build

```bash
sudo bash scripts/install_linux.sh
```

The first run will:

- install dependencies
- create the `tdi_reader` service user
- build the project into `build-linux`
- install `/etc/tdi_reader/config.yaml`
- register and start the `tdi_reader` systemd service

## 3. Manual build

```bash
cmake -S . -B build-linux
cmake --build build-linux --config Release -j"$(nproc)"
```

## 4. Service control

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

Status:

```bash
sudo systemctl status tdi_reader
```

Logs:

```bash
sudo journalctl -u tdi_reader -f
```

## 5. Update, rebuild, restart

Automatic:

```bash
sudo bash scripts/update_linux.sh
```

Manual:

```bash
git pull --ff-only
cmake -S . -B build-linux
cmake --build build-linux --config Release -j"$(nproc)"
sudo systemctl restart tdi_reader
```

## 6. Config

Main config file:

```bash
/etc/tdi_reader/config.yaml
```

After config changes:

```bash
sudo systemctl restart tdi_reader
```

## 7. Important note

Default config uses:

```yaml
serial_baud: 420000
```

The serial layer now tries exact `420000` on Linux through `termios2`/`BOTHER`.

If the kernel, FTDI driver, or adapter still rejects that rate, the app falls back to the nearest supported standard baudrate and writes a warning to `journalctl`.
