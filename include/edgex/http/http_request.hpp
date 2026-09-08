#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace edgex::http {

enum class HttpMethod {
    Get,
    Post,
    Put,
    Delete_,
    Head,
    Options,
    Patch,
};

enum class HttpVersion {
    Http10,
    Http11,
};

struct HeaderField {
    std::string name;
    std::string value;
};

class HttpRequest {
public:
    HttpMethod method{};
    std::string target;
    HttpVersion version{HttpVersion::Http11};
    std::vector<HeaderField> headers;
    std::optional<std::vector<std::uint8_t>> body;

    [[nodiscard]] bool has_header(std::string_view name) const;

    [[nodiscard]] std::optional<std::string_view> header_value(std::string_view name) const;
};

}  // namespace edgex::http
