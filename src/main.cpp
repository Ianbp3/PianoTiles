#include "VirtualPiano.hpp"
#include <iostream>

int main() {
    VirtualPiano app;
    if (!app.initialize()) {
        std::cerr << "Failed to initialize Piano Virtual.\n";
        return 1;
    }
    app.run();
    app.cleanup();
    return 0;
}
