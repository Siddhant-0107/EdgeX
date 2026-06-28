#include <iostream>

#include "core/application.hpp"
#include "utils/version.hpp"

int main() {
    const edgex::core::Application app{};
    std::cout << app.name() << " backend scaffold v" << edgex::utils::version() << '\n';
    return 0;
}
