#include "protocol/crsf_parser.hpp"

namespace protocol {

std::optional<std::vector<std::uint8_t>> CrsfParser::push(std::uint8_t byte) {
    buffer_.push_back(byte);

    if (buffer_.size() == 2) {
        const auto frame_length = buffer_[1];
        if (frame_length < 2 || frame_length > 62) {
            buffer_.clear();
        }
    }

    if (buffer_.size() >= 2) {
        const auto expected = static_cast<std::size_t>(buffer_[1]) + 2;
        if (buffer_.size() == expected) {
            auto frame = buffer_;
            buffer_.clear();
            if (validate_frame(frame)) {
                return frame;
            }
        } else if (buffer_.size() > expected) {
            buffer_.clear();
        }
    }

    return std::nullopt;
}

bool CrsfParser::validate_frame(const std::vector<std::uint8_t>& frame) {
    if (frame.size() < 4) {
        return false;
    }

    const auto length = static_cast<std::size_t>(frame[1]);
    if (length + 2 != frame.size()) {
        return false;
    }

    const auto computed_crc = crc8_dvb_s2(frame.data() + 2, frame.size() - 3);
    return computed_crc == frame.back();
}

std::uint8_t CrsfParser::crc8_dvb_s2(const std::uint8_t* data, std::size_t size) {
    std::uint8_t crc = 0;
    for (std::size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            if ((crc & 0x80U) != 0U) {
                crc = static_cast<std::uint8_t>((crc << 1U) ^ 0xD5U);
            } else {
                crc <<= 1U;
            }
        }
    }
    return crc;
}

}  // namespace protocol
