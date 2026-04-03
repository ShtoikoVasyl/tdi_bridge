#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <termios.h>
#include <vector>

namespace device {

class SerialPort {
  public:
    SerialPort() = default;
    ~SerialPort();

    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;

    bool open(const std::string& path, int baud_rate);
    void close();
    bool is_open() const;

    ssize_t read_some(std::uint8_t* buffer, std::size_t size);
    bool write_all(const std::vector<std::uint8_t>& data);

  private:
    bool configure(int baud_rate);
    static speed_t to_speed(int baud_rate);

    int fd_ = -1;
    std::atomic<bool> open_{false};
};

}  // namespace device
