#include <iostream>

#include "edgex/common/application.hpp"
#include "edgex/common/version.hpp"

int main() {
    const edgex::core::Application app{};
    std::cout << app.name() << " backend scaffold v" << edgex::utils::version() << '\n';
    return 0;
}
