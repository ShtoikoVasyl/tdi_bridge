#pragma once

#include "app/config.hpp"
#include "device/device_manager.hpp"
#include "device/serial_port.hpp"
#include "net/network_bridge.hpp"
#include "protocol/crsf_parser.hpp"

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

namespace core {

class BridgeController {
  public:
    explicit BridgeController(app::AppConfig config);
    ~BridgeController();

    bool start();
    void stop();
    bool is_running() const;

  private:
    bool connect_serial();
    void serial_loop();
    void handle_network_frame(const std::vector<std::uint8_t>& frame);
    std::unique_ptr<net::NetworkBridge> create_bridge() const;

    app::AppConfig config_;
    device::DeviceManager device_manager_;
    device::SerialPort serial_port_;
    protocol::CrsfParser parser_;
    std::unique_ptr<net::NetworkBridge> network_bridge_;
    std::atomic<bool> running_{false};
    std::thread serial_thread_;
};

}  // namespace core
