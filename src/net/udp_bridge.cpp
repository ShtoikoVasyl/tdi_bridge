#include "net/udp_bridge.hpp"

#include "app/logger.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace net {

UdpBridge::UdpBridge(std::string address, std::uint16_t port)
    : address_(std::move(address)),
      port_(port) {}

UdpBridge::~UdpBridge() {
    stop();
}

bool UdpBridge::start() {
    socket_fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    if (::inet_pton(AF_INET, address_.c_str(), &addr.sin_addr) != 1) {
        ::close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    if (::bind(socket_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        ::close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }

    running_.store(true);
    recv_thread_ = std::thread(&UdpBridge::recv_loop, this);
    app::Logger::instance().info("UDP bridge listening on " + address_ + ":" + std::to_string(port_));
    return true;
}

void UdpBridge::stop() {
    running_.store(false);

    if (socket_fd_ >= 0) {
        ::shutdown(socket_fd_, SHUT_RDWR);
        ::close(socket_fd_);
        socket_fd_ = -1;
    }

    if (recv_thread_.joinable()) {
        recv_thread_.join();
    }
}

bool UdpBridge::send_frame(const std::vector<std::uint8_t>& frame) {
    std::lock_guard<std::mutex> lock(peer_mutex_);
    if (socket_fd_ < 0 || !has_peer_) {
        return false;
    }

    const auto sent = ::sendto(socket_fd_,
                               frame.data(),
                               frame.size(),
                               0,
                               reinterpret_cast<sockaddr*>(&peer_addr_),
                               sizeof(peer_addr_));
    return sent == static_cast<ssize_t>(frame.size());
}

void UdpBridge::recv_loop() {
    std::vector<std::uint8_t> buffer(512);

    while (running_.load()) {
        sockaddr_in incoming{};
        socklen_t incoming_len = sizeof(incoming);
        const auto received = ::recvfrom(socket_fd_,
                                         buffer.data(),
                                         buffer.size(),
                                         0,
                                         reinterpret_cast<sockaddr*>(&incoming),
                                         &incoming_len);
        if (received <= 0) {
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(peer_mutex_);
            peer_addr_ = incoming;
            has_peer_ = true;
        }

        if (frame_handler_) {
            frame_handler_(std::vector<std::uint8_t>(buffer.begin(), buffer.begin() + received));
        }
    }
}

}  // namespace net
