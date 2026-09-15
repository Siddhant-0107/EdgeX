#include "edgex/http/http_response.hpp"

#include <algorithm>
#include <cctype>
#include <utility>

namespace edgex::http {

namespace {

bool ascii_case_insensitive_equal(std::string_view lhs,
                                  std::string_view rhs) noexcept {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (std::size_t i = 0; i < lhs.size(); ++i) {
        const unsigned char left =
            static_cast<unsigned char>(lhs[i]);
        const unsigned char right =
            static_cast<unsigned char>(rhs[i]);

        if (std::tolower(left) != std::tolower(right)) {
            return false;
        }
    }

    return true;
}

} // namespace

void HttpResponse::set_header(std::string name, std::string value) {
    for (auto& header : headers) {
        if (ascii_case_insensitive_equal(header.name, name)) {
            header.value = std::move(value);
            return;
        }
    }

    headers.push_back(
        ResponseHeader{std::move(name), std::move(value)});
}

bool HttpResponse::has_header(std::string_view name) const {
    return std::any_of(
        headers.begin(),
        headers.end(),
        [name](const ResponseHeader& header) {
            return ascii_case_insensitive_equal(header.name, name);
        });
}

std::string HttpResponse::serialize() const {
    std::string result;

    result += "HTTP/1.1 ";

    switch (status) {
        case HttpStatus::Ok:
            result += "200 OK";
            break;
        case HttpStatus::Created:
            result += "201 Created";
            break;
        case HttpStatus::NoContent:
            result += "204 No Content";
            break;
        case HttpStatus::BadRequest:
            result += "400 Bad Request";
            break;
        case HttpStatus::NotFound:
            result += "404 Not Found";
            break;
        case HttpStatus::MethodNotAllowed:
            result += "405 Method Not Allowed";
            break;
        case HttpStatus::InternalServerError:
            result += "500 Internal Server Error";
            break;
        case HttpStatus::NotImplemented:
            result += "501 Not Implemented";
            break;
        case HttpStatus::BadGateway:
            result += "502 Bad Gateway";
            break;
        case HttpStatus::ServiceUnavailable:
            result += "503 Service Unavailable";
            break;
    }

    result += "\r\n";

    for (const auto& header : headers) {
        result += header.name;
        result += ": ";
        result += header.value;
        result += "\r\n";
    }

    if (!has_header("Content-Length")) {
        result += "Content-Length: ";
        result += std::to_string(body.size());
        result += "\r\n";
    }

    result += "\r\n";

    result.append(
        reinterpret_cast<const char*>(body.data()),
        body.size());

    return result;
}

} // namespace edgex::http
