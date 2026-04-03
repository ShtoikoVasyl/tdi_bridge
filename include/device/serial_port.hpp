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
    int actual_baud_rate() const;

    ssize_t read_some(std::uint8_t* buffer, std::size_t size);
    bool write_all(const std::vector<std::uint8_t>& data);

  private:
    bool configure(int baud_rate);
    static bool apply_standard_termios(int fd, int baud_rate);
#if defined(__linux__)
    static bool apply_custom_linux_baud(int fd, int baud_rate);
#endif
    static speed_t to_speed(int baud_rate);
    static int nearest_supported_baud(int baud_rate);

    int fd_ = -1;
    int actual_baud_rate_ = 0;
    std::atomic<bool> open_{false};
};

}  // namespace device
