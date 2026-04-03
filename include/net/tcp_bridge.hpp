#pragma once

#include "net/network_bridge.hpp"

#include <mutex>
#include <thread>

namespace net {

class TcpBridge final : public NetworkBridge {
  public:
    TcpBridge(std::string address, std::uint16_t port);
    ~TcpBridge() override;

    bool start() override;
    void stop() override;
    bool send_frame(const std::vector<std::uint8_t>& frame) override;

  private:
    void accept_loop();
    void client_loop(int client_fd);
    bool send_all(int fd, const std::uint8_t* data, std::size_t size);
    bool recv_all(int fd, std::uint8_t* data, std::size_t size);

    std::string address_;
    std::uint16_t port_;
    int server_fd_ = -1;
    int client_fd_ = -1;
    std::thread accept_thread_;
    std::thread client_thread_;
    std::mutex client_mutex_;
};

}  // namespace net
