#include "net/network_bridge.hpp"

#include "net/tcp_bridge.hpp"
#include "net/udp_bridge.hpp"

namespace net {

void NetworkBridge::set_frame_handler(FrameHandler handler) {
    frame_handler_ = std::move(handler);
}

std::unique_ptr<NetworkBridge> create_udp_bridge(const std::string& address, std::uint16_t port) {
    return std::make_unique<UdpBridge>(address, port);
}

std::unique_ptr<NetworkBridge> create_tcp_bridge(const std::string& address, std::uint16_t port) {
    return std::make_unique<TcpBridge>(address, port);
}

}  // namespace net
