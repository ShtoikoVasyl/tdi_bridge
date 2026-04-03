#pragma once

#include "net/network_bridge.hpp"

#include <netinet/in.h>
#include <mutex>
#include <thread>

namespace net {

class UdpBridge final : public NetworkBridge {
  public:
    UdpBridge(std::string address, std::uint16_t port);
    ~UdpBridge() override;

    bool start() override;
    void stop() override;
    bool send_frame(const std::vector<std::uint8_t>& frame) override;

  private:
    void recv_loop();

    std::string address_;
    std::uint16_t port_;
    int socket_fd_ = -1;
    std::thread recv_thread_;
    std::mutex peer_mutex_;
    bool has_peer_ = false;
    sockaddr_in peer_addr_{};
};

}  // namespace net
