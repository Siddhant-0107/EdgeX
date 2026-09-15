#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "edgex/http/http_response.hpp"

namespace edgex::http {

class HttpResponseBuilder {
public:
    HttpResponseBuilder& status(HttpStatus status);

    HttpResponseBuilder& header(std::string name, std::string value);

    HttpResponseBuilder& body(std::string_view body);

    HttpResponseBuilder& body(const std::vector<std::uint8_t>& body);

    [[nodiscard]]
    HttpResponse build() const;

private:
    HttpResponse response_{};
};

} // namespace edgex::http
