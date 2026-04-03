#include "core/bridge_controller.hpp"

#include "app/logger.hpp"

#include <chrono>
#include <thread>

namespace core {

BridgeController::BridgeController(app::AppConfig config)
    : config_(std::move(config)),
      device_manager_(config_.serial_device) {}

BridgeController::~BridgeController() {
    stop();
}

bool BridgeController::start() {
    network_bridge_ = create_bridge();
    if (!network_bridge_) {
        app::Logger::instance().error("Failed to create network bridge");
        return false;
    }

    network_bridge_->set_frame_handler([this](const std::vector<std::uint8_t>& frame) {
        handle_network_frame(frame);
    });

    if (!network_bridge_->start()) {
        app::Logger::instance().error("Failed to start network bridge");
        return false;
    }

    running_.store(true);
    serial_thread_ = std::thread(&BridgeController::serial_loop, this);
    return true;
}

void BridgeController::stop() {
    running_.store(false);
    serial_port_.close();

    if (network_bridge_) {
        network_bridge_->stop();
    }

    if (serial_thread_.joinable()) {
        serial_thread_.join();
    }
}

bool BridgeController::is_running() const {
    return running_.load();
}

bool BridgeController::connect_serial() {
    const auto device = device_manager_.resolve_device();
    if (!device.has_value()) {
        app::Logger::instance().warn("No FTDI/serial device found");
        return false;
    }

    if (!serial_port_.open(*device, config_.serial_baud)) {
        app::Logger::instance().error("Failed to open serial port: " + *device);
        return false;
    }

    app::Logger::instance().info("Serial connected: " + *device + " @ " + std::to_string(config_.serial_baud));
    return true;
}

void BridgeController::serial_loop() {
    std::vector<std::uint8_t> buffer(256);

    while (running_.load()) {
        if (!serial_port_.is_open() && !connect_serial()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(config_.reconnect_delay_ms));
            continue;
        }

        const auto bytes = serial_port_.read_some(buffer.data(), buffer.size());
        if (bytes <= 0) {
            app::Logger::instance().warn("Serial read failed, reconnecting");
            serial_port_.close();
            std::this_thread::sleep_for(std::chrono::milliseconds(config_.reconnect_delay_ms));
            continue;
        }

        if (config_.raw_mode) {
            network_bridge_->send_frame(std::vector<std::uint8_t>(buffer.begin(), buffer.begin() + bytes));
            continue;
        }

        for (ssize_t i = 0; i < bytes; ++i) {
            auto frame = parser_.push(buffer[static_cast<std::size_t>(i)]);
            if (frame.has_value()) {
                network_bridge_->send_frame(frame.value());
            }
        }
    }
}

void BridgeController::handle_network_frame(const std::vector<std::uint8_t>& frame) {
    if (!serial_port_.is_open()) {
        app::Logger::instance().warn("Dropping network frame because serial is not connected");
        return;
    }

    if (!config_.raw_mode && !protocol::CrsfParser::validate_frame(frame)) {
        app::Logger::instance().warn("Dropping invalid CRSF frame from network");
        return;
    }

    if (!serial_port_.write_all(frame)) {
        app::Logger::instance().warn("Failed to forward frame to serial");
        serial_port_.close();
    }
}

std::unique_ptr<net::NetworkBridge> BridgeController::create_bridge() const {
    if (config_.transport == app::TransportMode::Udp) {
        return net::create_udp_bridge(config_.listen_address, config_.listen_port);
    }
    return net::create_tcp_bridge(config_.listen_address, config_.listen_port);
}

}  // namespace core
