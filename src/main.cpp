#include "app/config.hpp"
#include "app/logger.hpp"
#include "core/bridge_controller.hpp"

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

namespace {

std::atomic<bool> keep_running = true;

void handle_signal(int) {
    keep_running.store(false);
}

std::string hex_or_auto(const std::optional<std::uint16_t>& value) {
    if (!value.has_value()) {
        return "auto";
    }

    std::ostringstream stream;
    stream << "0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << value.value();
    return stream.str();
}

std::string startup_summary(const app::AppConfig& config, const std::string& config_path) {
    std::ostringstream stream;
    stream << "Startup configuration:\n";
    stream << "  config: " << config_path << '\n';
    stream << "  transport: " << app::to_string(config.transport) << '\n';
    stream << "  listen: " << config.listen_address << ':' << config.listen_port << '\n';
    stream << "  serial baud: " << config.serial_baud << '\n';
    stream << "  mode: " << (config.raw_mode ? "raw bridge" : "CRSF framing") << '\n';
    stream << "  device path: "
           << (config.serial_device.explicit_device.empty() ? "auto" : config.serial_device.explicit_device) << '\n';
    stream << "  device hint: "
           << (config.serial_device.device_hint.empty() ? "none" : config.serial_device.device_hint) << '\n';
    stream << "  usb vid: " << hex_or_auto(config.serial_device.vendor_id) << '\n';
    stream << "  usb pid: " << hex_or_auto(config.serial_device.product_id) << '\n';
    stream << "  reconnect delay ms: " << config.reconnect_delay_ms << '\n';
    stream << "  log level: " << config.log_level;
    return stream.str();
}

}  // namespace

int main(int argc, char** argv) {
    const std::string config_path = argc > 1 ? argv[1] : "config/config.yaml";

    try {
        const auto config = app::load_config(config_path);
        app::Logger::instance().set_level(app::parse_log_level(config.log_level));
        app::Logger::instance().info("Starting tdi_reader in " + app::to_string(config.transport) + " mode");
        app::Logger::instance().info(startup_summary(config, config_path));

        std::signal(SIGINT, handle_signal);
        std::signal(SIGTERM, handle_signal);

        core::BridgeController controller(config);
        if (!controller.start()) {
            app::Logger::instance().error("Startup failed");
            return EXIT_FAILURE;
        }

        while (keep_running.load() && controller.is_running()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }

        controller.stop();
        app::Logger::instance().info("Shutdown complete");
        return EXIT_SUCCESS;
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << '\n';
        return EXIT_FAILURE;
    }
}
