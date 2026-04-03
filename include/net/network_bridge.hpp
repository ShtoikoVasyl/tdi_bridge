#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace net {

using FrameHandler = std::function<void(const std::vector<std::uint8_t>&)>;

class NetworkBridge {
  public:
    virtual ~NetworkBridge() = default;

    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool send_frame(const std::vector<std::uint8_t>& frame) = 0;

    void set_frame_handler(FrameHandler handler);

  protected:
    std::atomic<bool> running_{false};
    FrameHandler frame_handler_;
};

std::unique_ptr<NetworkBridge> create_udp_bridge(const std::string& address, std::uint16_t port);
std::unique_ptr<NetworkBridge> create_tcp_bridge(const std::string& address, std::uint16_t port);

}  // namespace net
