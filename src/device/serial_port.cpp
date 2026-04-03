#include "device/serial_port.hpp"

#include "app/logger.hpp"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstring>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#if defined(__linux__)
#include <asm/termbits.h>
#include <sys/ioctl.h>
#endif

namespace device {

bool SerialPort::apply_standard_termios(int fd, int baud_rate) {
    termios tty{};
    if (tcgetattr(fd, &tty) != 0) {
        return false;
    }

    cfmakeraw(&tty);
    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    const auto speed = SerialPort::to_speed(baud_rate);
    if (speed == 0) {
        return false;
    }

    if (cfsetispeed(&tty, speed) != 0 || cfsetospeed(&tty, speed) != 0) {
        return false;
    }

    return tcsetattr(fd, TCSANOW, &tty) == 0;
}

#if defined(__linux__)
bool SerialPort::apply_custom_linux_baud(int fd, int baud_rate) {
    termios2 tty{};
    if (ioctl(fd, TCGETS2, &tty) != 0) {
        return false;
    }

    tty.c_cflag &= ~CBAUD;
    tty.c_cflag |= BOTHER | CLOCAL | CREAD;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_iflag = 0;
    tty.c_oflag = 0;
    tty.c_lflag = 0;
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;
    tty.c_ispeed = baud_rate;
    tty.c_ospeed = baud_rate;

    return ioctl(fd, TCSETS2, &tty) == 0;
}
#endif

SerialPort::~SerialPort() {
    close();
}

bool SerialPort::open(const std::string& path, int baud_rate) {
    close();

    fd_ = ::open(path.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd_ < 0) {
        return false;
    }

    if (!configure(baud_rate)) {
        close();
        return false;
    }

    open_.store(true);
    return true;
}

void SerialPort::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
    actual_baud_rate_ = 0;
    open_.store(false);
}

bool SerialPort::is_open() const {
    return open_.load();
}

int SerialPort::actual_baud_rate() const {
    return actual_baud_rate_;
}

ssize_t SerialPort::read_some(std::uint8_t* buffer, std::size_t size) {
    if (fd_ < 0) {
        errno = EBADF;
        return -1;
    }
    return ::read(fd_, buffer, size);
}

bool SerialPort::write_all(const std::vector<std::uint8_t>& data) {
    if (fd_ < 0) {
        return false;
    }

    std::size_t offset = 0;
    while (offset < data.size()) {
        const auto written = ::write(fd_, data.data() + offset, data.size() - offset);
        if (written < 0) {
            return false;
        }
        offset += static_cast<std::size_t>(written);
    }
    return true;
}

bool SerialPort::configure(int baud_rate) {
    if (apply_standard_termios(fd_, baud_rate)) {
        actual_baud_rate_ = baud_rate;
        app::Logger::instance().info("Serial baud configured: requested=" + std::to_string(baud_rate) +
                                     ", applied=" + std::to_string(actual_baud_rate_));
        return true;
    }

#if defined(__linux__)
    if (apply_custom_linux_baud(fd_, baud_rate)) {
        actual_baud_rate_ = baud_rate;
        app::Logger::instance().info("Serial baud configured via termios2/BOTHER: requested=" +
                                     std::to_string(baud_rate) + ", applied=" + std::to_string(actual_baud_rate_));
        return true;
    }
#endif

    const auto fallback_baud = nearest_supported_baud(baud_rate);
    if (fallback_baud > 0 && fallback_baud != baud_rate && apply_standard_termios(fd_, fallback_baud)) {
        actual_baud_rate_ = fallback_baud;
        app::Logger::instance().warn("Requested serial baud " + std::to_string(baud_rate) +
                                     " is unavailable, falling back to " + std::to_string(actual_baud_rate_));
        return true;
    }

    actual_baud_rate_ = 0;
    app::Logger::instance().error("Unable to configure serial baud " + std::to_string(baud_rate));
    return false;
}

speed_t SerialPort::to_speed(int baud_rate) {
    switch (baud_rate) {
        case 9600:
            return B9600;
        case 19200:
            return B19200;
        case 38400:
            return B38400;
        case 57600:
            return B57600;
        case 115200:
            return B115200;
        case 230400:
            return B230400;
#ifdef B460800
        case 460800:
            return B460800;
#endif
#ifdef B500000
        case 500000:
            return B500000;
#endif
#ifdef B576000
        case 576000:
            return B576000;
#endif
#ifdef B921600
        case 921600:
            return B921600;
#endif
        default:
            return 0;
    }
}

int SerialPort::nearest_supported_baud(int baud_rate) {
    constexpr int supported_bauds[] = {
        9600,
        19200,
        38400,
        57600,
        115200,
        230400,
#ifdef B460800
        460800,
#endif
#ifdef B500000
        500000,
#endif
#ifdef B576000
        576000,
#endif
#ifdef B921600
        921600,
#endif
    };

    int best_baud = 0;
    int best_distance = INT_MAX;

    for (const auto candidate : supported_bauds) {
        const auto distance = std::abs(candidate - baud_rate);
        if (distance < best_distance) {
            best_distance = distance;
            best_baud = candidate;
        }
    }

    return best_baud;
}

}  // namespace device
