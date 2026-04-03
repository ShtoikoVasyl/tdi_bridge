#include "device/device_manager.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

namespace device {

namespace fs = std::filesystem;

DeviceManager::DeviceManager(app::DeviceConfig config)
    : config_(std::move(config)) {}

std::optional<std::string> DeviceManager::resolve_device() const {
    if (!config_.explicit_device.empty() && fs::exists(config_.explicit_device)) {
        return config_.explicit_device;
    }

    const std::vector<fs::path> roots = {
        "/dev/serial/by-id",
        "/dev"
    };

    for (const auto& root : roots) {
        if (!fs::exists(root) || !fs::is_directory(root)) {
            continue;
        }

        for (const auto& entry : fs::directory_iterator(root)) {
            const auto candidate = entry.path().string();
            const auto filename = entry.path().filename().string();
            const bool tty_match = filename.rfind("ttyUSB", 0) == 0 || filename.rfind("ttyACM", 0) == 0;
            const bool serial_symlink = root == fs::path("/dev/serial/by-id");

            if (!tty_match && !serial_symlink) {
                continue;
            }

            if (!matches_hint(candidate)) {
                continue;
            }

            if (!matches_usb_ids(candidate)) {
                continue;
            }

            return serial_symlink ? fs::canonical(entry.path()).string() : candidate;
        }
    }

    return std::nullopt;
}

bool DeviceManager::matches_hint(const std::string& candidate) const {
    if (config_.device_hint.empty()) {
        return true;
    }

    auto lowered = candidate;
    auto hint = config_.device_hint;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    std::transform(hint.begin(), hint.end(), hint.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return lowered.find(hint) != std::string::npos;
}

bool DeviceManager::matches_usb_ids(const std::string& candidate) const {
    if (!config_.vendor_id.has_value() && !config_.product_id.has_value()) {
        return true;
    }

    const auto sys_name = fs::path(candidate).filename().string();
    const fs::path sys_base = fs::path("/sys/class/tty") / sys_name / "device/../..";
    const fs::path vendor_path = fs::weakly_canonical(sys_base / "idVendor");
    const fs::path product_path = fs::weakly_canonical(sys_base / "idProduct");

    if (!fs::exists(vendor_path) || !fs::exists(product_path)) {
        return false;
    }

    std::ifstream vendor_stream(vendor_path);
    std::ifstream product_stream(product_path);
    std::string vendor;
    std::string product;
    vendor_stream >> vendor;
    product_stream >> product;

    const auto vendor_value = static_cast<std::uint16_t>(std::stoul(vendor, nullptr, 16));
    const auto product_value = static_cast<std::uint16_t>(std::stoul(product, nullptr, 16));

    if (config_.vendor_id.has_value() && vendor_value != config_.vendor_id.value()) {
        return false;
    }

    if (config_.product_id.has_value() && product_value != config_.product_id.value()) {
        return false;
    }

    return true;
}

}  // namespace device
