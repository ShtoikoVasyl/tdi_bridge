# tdi_reader

Linux-oriented C++ skeleton for an FTDI to CRSF bridge that exposes a network endpoint on port `9000`.

## Features

- FTDI/TTY device discovery using `/dev/serial/by-id` and `/dev/ttyUSB*`
- serial port open/configure via `termios`
- CRSF frame parser with CRC-8 DVB-S2 validation
- UDP bridge mode where one datagram carries one CRSF frame
- TCP bridge mode with a 16-bit big-endian length prefix
- reconnect loop for serial device loss/recovery
- single-process daemon-friendly architecture

## Config

The application reads `config/config.yaml`.

Example:

```yaml
transport: udp
listen_address: 0.0.0.0
listen_port: 9000
serial_baud: 420000
serial_device:
device_hint: ftdi
vendor_id: 0x0403
product_id: 0x6001
raw_mode: false
log_level: info
reconnect_delay_ms: 2000
```

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Run

```bash
./build/tdi_reader config/config.yaml
```
