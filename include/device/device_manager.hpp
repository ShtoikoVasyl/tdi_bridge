#pragma once

#include "app/config.hpp"

#include <optional>
#include <string>

namespace device {

class DeviceManager {
  public:
    explicit DeviceManager(app::DeviceConfig config);

    std::optional<std::string> resolve_device() const;

  private:
    bool matches_hint(const std::string& candidate) const;
    bool matches_usb_ids(const std::string& candidate) const;

    app::DeviceConfig config_;
};

}  // namespace device
