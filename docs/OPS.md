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
- use your current Linux user as the service user when the repo lives under `/home/<user>/...`
- otherwise create and use the `tdi_reader` service user
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
bash scripts/update_linux.sh
```

Manual:

```bash
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

## 7. Useful checks

Show the active service unit:

```bash
systemctl cat tdi_reader
```

Show service status with full errors:

```bash
sudo systemctl status tdi_reader --no-pager -l
```

Show recent logs:

```bash
sudo journalctl -u tdi_reader -n 100 --no-pager
```

Follow logs in realtime:

```bash
sudo journalctl -u tdi_reader -f
```

Show the real runtime config file:

```bash
sudo cat /etc/tdi_reader/config.yaml
```

Check whether port `9000` is listening:

```bash
ss -ltnup | grep 9000
```

Check only TCP listener:

```bash
ss -ltn | grep 9000
```

Check only UDP listener:

```bash
ss -lun | grep 9000
```

Check FTDI/USB serial devices:

```bash
ls /dev/ttyUSB*
ls -l /dev/serial/by-id
```

Check whether the FTDI driver is loaded:

```bash
lsmod | grep ftdi_sio
```

Show USB devices:

```bash
lsusb
```

Check access rights for the serial device:

```bash
id
groups
ls -l /dev/ttyUSB0
```

Check firewall state:

```bash
sudo ufw status
sudo iptables -L -n
```

Check path permissions if systemd cannot `chdir`:

```bash
namei -l /home/$USER/Desktop/tdi_bridge
```

## 8. Important note

Default config uses:

```yaml
serial_baud: 420000
```

The serial layer now tries exact `420000` on Linux through `termios2`/`BOTHER`.

If the kernel, FTDI driver, or adapter still rejects that rate, the app falls back to the nearest supported standard baudrate and writes a warning to `journalctl`.
