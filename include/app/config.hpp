#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace app {

enum class TransportMode {
    Udp,
    Tcp
};

struct DeviceConfig {
    std::string explicit_device;
    std::string device_hint = "ftdi";
    std::optional<std::uint16_t> vendor_id;
    std::optional<std::uint16_t> product_id;
};

struct AppConfig {
    TransportMode transport = TransportMode::Udp;
    std::string listen_address = "0.0.0.0";
    std::uint16_t listen_port = 9000;
    int serial_baud = 420000;
    DeviceConfig serial_device;
    bool raw_mode = false;
    std::string log_level = "info";
    int reconnect_delay_ms = 2000;
};

AppConfig load_config(const std::string& path);
std::string to_string(TransportMode mode);

}  // namespace app
