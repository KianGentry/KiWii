#include "mkwii/config.h"
#include "mkwii/server.h"

#include <exception>
#include <iostream>

// load configuration from environment and run the server
int main() {
    try {
        const mkwii::Config config = mkwii::config_from_environment();
        return mkwii::run_server(config);
    } catch (const std::exception& error) {
        std::cerr << "configuration error: " << error.what() << '\n';
        return 1;
    }
}
