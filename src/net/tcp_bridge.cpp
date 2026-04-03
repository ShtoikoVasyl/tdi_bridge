#include "net/tcp_bridge.hpp"

#include "app/logger.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace net {

TcpBridge::TcpBridge(std::string address, std::uint16_t port)
    : address_(std::move(address)),
      port_(port) {}

TcpBridge::~TcpBridge() {
    stop();
}

bool TcpBridge::start() {
    server_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        return false;
    }

    int enable = 1;
    ::setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    if (::inet_pton(AF_INET, address_.c_str(), &addr.sin_addr) != 1) {
        ::close(server_fd_);
        server_fd_ = -1;
        return false;
    }

    if (::bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        ::close(server_fd_);
        server_fd_ = -1;
        return false;
    }

    if (::listen(server_fd_, 1) != 0) {
        ::close(server_fd_);
        server_fd_ = -1;
        return false;
    }

    running_.store(true);
    accept_thread_ = std::thread(&TcpBridge::accept_loop, this);
    app::Logger::instance().info("TCP bridge listening on " + address_ + ":" + std::to_string(port_));
    return true;
}

void TcpBridge::stop() {
    running_.store(false);

    {
        std::lock_guard<std::mutex> lock(client_mutex_);
        if (client_fd_ >= 0) {
            ::shutdown(client_fd_, SHUT_RDWR);
            ::close(client_fd_);
            client_fd_ = -1;
        }
    }

    if (server_fd_ >= 0) {
        ::shutdown(server_fd_, SHUT_RDWR);
        ::close(server_fd_);
        server_fd_ = -1;
    }

    if (client_thread_.joinable()) {
        client_thread_.join();
    }
    if (accept_thread_.joinable()) {
        accept_thread_.join();
    }
}

bool TcpBridge::send_frame(const std::vector<std::uint8_t>& frame) {
    std::lock_guard<std::mutex> lock(client_mutex_);
    if (client_fd_ < 0 || frame.size() > 0xFFFF) {
        return false;
    }

    const std::uint16_t size = htons(static_cast<std::uint16_t>(frame.size()));
    return send_all(client_fd_, reinterpret_cast<const std::uint8_t*>(&size), sizeof(size)) &&
           send_all(client_fd_, frame.data(), frame.size());
}

void TcpBridge::accept_loop() {
    while (running_.load()) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        const int accepted = ::accept(server_fd_, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
        if (accepted < 0) {
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(client_mutex_);
            if (client_fd_ >= 0) {
                ::shutdown(client_fd_, SHUT_RDWR);
                ::close(client_fd_);
            }
            client_fd_ = accepted;
        }

        if (client_thread_.joinable()) {
            client_thread_.join();
        }
        client_thread_ = std::thread(&TcpBridge::client_loop, this, accepted);
    }
}

void TcpBridge::client_loop(int client_fd) {
    while (running_.load()) {
        std::uint16_t network_size = 0;
        if (!recv_all(client_fd, reinterpret_cast<std::uint8_t*>(&network_size), sizeof(network_size))) {
            break;
        }

        const auto size = ntohs(network_size);
        if (size == 0) {
            continue;
        }

        std::vector<std::uint8_t> frame(size);
        if (!recv_all(client_fd, frame.data(), frame.size())) {
            break;
        }

        if (frame_handler_) {
            frame_handler_(frame);
        }
    }

    std::lock_guard<std::mutex> lock(client_mutex_);
    if (client_fd_ == client_fd) {
        ::close(client_fd_);
        client_fd_ = -1;
    }
}

bool TcpBridge::send_all(int fd, const std::uint8_t* data, std::size_t size) {
    std::size_t offset = 0;
    while (offset < size) {
        const auto sent = ::send(fd, data + offset, size - offset, 0);
        if (sent <= 0) {
            return false;
        }
        offset += static_cast<std::size_t>(sent);
    }
    return true;
}

bool TcpBridge::recv_all(int fd, std::uint8_t* data, std::size_t size) {
    std::size_t offset = 0;
    while (offset < size) {
        const auto received = ::recv(fd, data + offset, size - offset, 0);
        if (received <= 0) {
            return false;
        }
        offset += static_cast<std::size_t>(received);
    }
    return true;
}

}  // namespace net
