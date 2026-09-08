#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace edgex::http {

enum class HttpStatus {
    Ok = 200,
    Created = 201,
    NoContent = 204,

    BadRequest = 400,
    NotFound = 404,
    MethodNotAllowed = 405,

    InternalServerError = 500,
    NotImplemented = 501,
    BadGateway = 502,
    ServiceUnavailable = 503,
};

struct ResponseHeader {
    std::string name;
    std::string value;
};

class HttpResponse {
public:
    HttpStatus status{HttpStatus::Ok};
    std::vector<ResponseHeader> headers;
    std::vector<std::uint8_t> body;

    void set_header(std::string name, std::string value);

    [[nodiscard]]
    bool has_header(std::string_view name) const;

    [[nodiscard]]
    std::string serialize() const;
};

} // namespace edgex::http
