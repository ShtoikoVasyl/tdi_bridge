#include "device/serial_port.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

namespace device {

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
    open_.store(false);
}

bool SerialPort::is_open() const {
    return open_.load();
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
    termios tty{};
    if (tcgetattr(fd_, &tty) != 0) {
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

    const auto speed = to_speed(baud_rate);
    if (speed == 0) {
        return false;
    }

    if (cfsetispeed(&tty, speed) != 0 || cfsetospeed(&tty, speed) != 0) {
        return false;
    }

    if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
        return false;
    }

    return true;
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

}  // namespace device
