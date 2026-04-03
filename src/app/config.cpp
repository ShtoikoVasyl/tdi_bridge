#include "app/config.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace app {

namespace {

std::string trim(const std::string& input) {
    const auto first = std::find_if_not(input.begin(), input.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
    const auto last = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char c) {
        return std::isspace(c) != 0;
    }).base();

    if (first >= last) {
        return {};
    }

    return std::string(first, last);
}

bool parse_bool(const std::string& value) {
    const auto lowered = [&]() {
        std::string copy = value;
        std::transform(copy.begin(), copy.end(), copy.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return copy;
    }();

    return lowered == "true" || lowered == "1" || lowered == "yes" || lowered == "on";
}

std::optional<std::uint16_t> parse_u16_optional(const std::string& value) {
    if (value.empty()) {
        return std::nullopt;
    }

    return static_cast<std::uint16_t>(std::stoul(value, nullptr, 0));
}

}  // namespace

AppConfig load_config(const std::string& path) {
    std::ifstream stream(path);
    if (!stream) {
        throw std::runtime_error("Unable to open config file: " + path);
    }

    AppConfig config;
    bool in_serial_device = false;
    std::string line;

    while (std::getline(stream, line)) {
        const auto comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }

        if (trim(line).empty()) {
            continue;
        }

        const auto indent = line.find_first_not_of(' ');
        const auto content = trim(line);

        if (content == "serial_device:") {
            in_serial_device = true;
            continue;
        }

        if (indent == 0 && content.back() != ':') {
            in_serial_device = false;
        }

        const auto separator = content.find(':');
        if (separator == std::string::npos) {
            continue;
        }

        const auto key = trim(content.substr(0, separator));
        const auto value = trim(content.substr(separator + 1));

        if (in_serial_device && indent != 0) {
            if (key == "device") {
                config.serial_device.explicit_device = value;
            } else if (key == "device_hint") {
                config.serial_device.device_hint = value;
            } else if (key == "vendor_id") {
                config.serial_device.vendor_id = parse_u16_optional(value);
            } else if (key == "product_id") {
                config.serial_device.product_id = parse_u16_optional(value);
            }
            continue;
        }

        if (key == "transport") {
            if (value == "udp") {
                config.transport = TransportMode::Udp;
            } else if (value == "tcp") {
                config.transport = TransportMode::Tcp;
            } else {
                throw std::runtime_error("Unsupported transport: " + value);
            }
        } else if (key == "listen_address") {
            config.listen_address = value;
        } else if (key == "listen_port") {
            config.listen_port = static_cast<std::uint16_t>(std::stoul(value));
        } else if (key == "serial_baud") {
            config.serial_baud = std::stoi(value);
        } else if (key == "raw_mode") {
            config.raw_mode = parse_bool(value);
        } else if (key == "log_level") {
            config.log_level = value;
        } else if (key == "reconnect_delay_ms") {
            config.reconnect_delay_ms = std::stoi(value);
        }
    }

    return config;
}

std::string to_string(TransportMode mode) {
    switch (mode) {
        case TransportMode::Udp:
            return "udp";
        case TransportMode::Tcp:
            return "tcp";
    }
    return "unknown";
}

}  // namespace app
