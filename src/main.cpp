#include "app/config.hpp"
#include "app/logger.hpp"
#include "core/bridge_controller.hpp"

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <thread>

namespace {

std::atomic<bool> keep_running = true;

void handle_signal(int) {
    keep_running.store(false);
}

}  // namespace

int main(int argc, char** argv) {
    const std::string config_path = argc > 1 ? argv[1] : "config/config.yaml";

    try {
        const auto config = app::load_config(config_path);
        app::Logger::instance().set_level(app::parse_log_level(config.log_level));
        app::Logger::instance().info("Starting tdi_reader in " + app::to_string(config.transport) + " mode");

        std::signal(SIGINT, handle_signal);
        std::signal(SIGTERM, handle_signal);

        core::BridgeController controller(config);
        if (!controller.start()) {
            app::Logger::instance().error("Startup failed");
            return EXIT_FAILURE;
        }

        while (keep_running.load() && controller.is_running()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }

        controller.stop();
        app::Logger::instance().info("Shutdown complete");
        return EXIT_SUCCESS;
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << '\n';
        return EXIT_FAILURE;
    }
}
