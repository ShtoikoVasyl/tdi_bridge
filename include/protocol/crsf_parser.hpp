#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace protocol {

class CrsfParser {
  public:
    std::optional<std::vector<std::uint8_t>> push(std::uint8_t byte);
    static bool validate_frame(const std::vector<std::uint8_t>& frame);
    static std::uint8_t crc8_dvb_s2(const std::uint8_t* data, std::size_t size);

  private:
    std::vector<std::uint8_t> buffer_;
};

}  // namespace protocol
