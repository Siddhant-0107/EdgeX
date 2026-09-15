#include "edgex/proxy/reverse_proxy.hpp"

#include <algorithm>
#include <cctype>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "edgex/net/tcp_client.hpp"

namespace edgex::proxy {

namespace {

bool equals_ci(std::string_view lhs, std::string_view rhs) noexcept {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (std::size_t i = 0; i < lhs.size(); ++i) {
        const auto left = static_cast<unsigned char>(lhs[i]);
        const auto right = static_cast<unsigned char>(rhs[i]);
        if (std::tolower(left) != std::tolower(right)) {
            return false;
        }
    }
    return true;
}

bool is_hop_by_hop_header(std::string_view name) noexcept {
    return equals_ci(name, "Connection") ||
           equals_ci(name, "Keep-Alive") ||
           equals_ci(name, "Proxy-Authenticate") ||
           equals_ci(name, "Proxy-Authorization") ||
           equals_ci(name, "TE") ||
           equals_ci(name, "Trailer") ||
           equals_ci(name, "Transfer-Encoding") ||
           equals_ci(name, "Upgrade");
}

std::string method_to_string(http::HttpMethod method) {
    switch (method) {
        case http::HttpMethod::Get: return "GET";
        case http::HttpMethod::Post: return "POST";
        case http::HttpMethod::Put: return "PUT";
        case http::HttpMethod::Delete_: return "DELETE";
        case http::HttpMethod::Head: return "HEAD";
        case http::HttpMethod::Options: return "OPTIONS";
        case http::HttpMethod::Patch: return "PATCH";
    }
    throw std::invalid_argument("unsupported HTTP method");
}

std::string serialize_request(const http::HttpRequest& request,
                              const ReverseProxyConfig& config) {
    if (request.target.empty() || request.target.front() != '/') {
        throw std::invalid_argument("reverse proxy requires an origin-form request target");
    }

    std::string result = method_to_string(request.method);
    result += ' ';
    result += request.target;
    result += " HTTP/1.1\r\n";

    bool has_host = false;
    bool has_content_length = false;

    for (const auto& header : request.headers) {
        if (is_hop_by_hop_header(header.name)) {
            continue;
        }
        if (equals_ci(header.name, "Host")) {
            has_host = true;
        }
        if (equals_ci(header.name, "Content-Length")) {
            has_content_length = true;
        }
        result += header.name;
        result += ": ";
        result += header.value;
        result += "\r\n";
    }

    if (!has_host) {
        result += "Host: ";
        result += config.upstream_address;
        result += ':';
        result += std::to_string(config.upstream_port);
        result += "\r\n";
    }

    const std::size_t body_size = request.body ? request.body->size() : 0;
    if (body_size != 0 && !has_content_length) {
        result += "Content-Length: ";
        result += std::to_string(body_size);
        result += "\r\n";
    }

    result += "Connection: close\r\n\r\n";

    if (request.body && !request.body->empty()) {
        result.append(reinterpret_cast<const char*>(request.body->data()),
                      request.body->size());
    }

    return result;
}

std::size_t find_header_end(const std::vector<std::uint8_t>& data) {
    constexpr std::array<std::uint8_t, 4> marker{{'\r', '\n', '\r', '\n'}};
    if (data.size() < marker.size()) {
        return std::string::npos;
    }
    const auto it = std::search(data.begin(), data.end(), marker.begin(), marker.end());
    if (it == data.end()) {
        return std::string::npos;
    }
    return static_cast<std::size_t>(std::distance(data.begin(), it));
}

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t");
    return value.substr(first, last - first + 1);
}

http::HttpStatus status_from_code(int code) {
    switch (code) {
        case 200: return http::HttpStatus::Ok;
        case 201: return http::HttpStatus::Created;
        case 204: return http::HttpStatus::NoContent;
        case 400: return http::HttpStatus::BadRequest;
        case 404: return http::HttpStatus::NotFound;
        case 405: return http::HttpStatus::MethodNotAllowed;
        case 500: return http::HttpStatus::InternalServerError;
        case 501: return http::HttpStatus::NotImplemented;
        case 502: return http::HttpStatus::BadGateway;
        case 503: return http::HttpStatus::ServiceUnavailable;
        default: throw std::runtime_error("unsupported upstream HTTP status");
    }
}

http::HttpResponse parse_response(const std::vector<std::uint8_t>& data) {
    const std::size_t header_end = find_header_end(data);
    if (header_end == std::string::npos) {
        throw std::runtime_error("incomplete upstream response headers");
    }

    const std::string header_text(
        reinterpret_cast<const char*>(data.data()), header_end);
    std::istringstream stream(header_text);

    std::string status_line;
    if (!std::getline(stream, status_line)) {
        throw std::runtime_error("missing upstream status line");
    }
    if (!status_line.empty() && status_line.back() == '\r') {
        status_line.pop_back();
    }

    std::istringstream status_stream(status_line);
    std::string version;
    int status_code = 0;
    if (!(status_stream >> version >> status_code) || version != "HTTP/1.1" ||
        status_code < 100 || status_code > 599) {
        throw std::runtime_error("malformed upstream status line");
    }

    http::HttpResponse response;
    response.status = status_from_code(status_code);

    std::size_t content_length = 0;
    bool has_content_length = false;

    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }

        const std::size_t colon = line.find(':');
        if (colon == std::string::npos || colon == 0) {
            throw std::runtime_error("malformed upstream header");
        }

        std::string name = line.substr(0, colon);
        std::string value = trim(line.substr(colon + 1));

        if (is_hop_by_hop_header(name)) {
            continue;
        }

        if (equals_ci(name, "Content-Length")) {
            try {
                const unsigned long long parsed = std::stoull(value);
                if (parsed > std::numeric_limits<std::size_t>::max()) {
                    throw std::runtime_error("upstream Content-Length is too large");
                }
                content_length = static_cast<std::size_t>(parsed);
                has_content_length = true;
            } catch (const std::exception&) {
                throw std::runtime_error("invalid upstream Content-Length");
            }
        }

        response.headers.push_back({std::move(name), std::move(value)});
    }

    const std::size_t body_start = header_end + 4;
    const std::size_t available = data.size() - body_start;
    if (has_content_length) {
        if (available < content_length) {
            throw std::runtime_error("incomplete upstream response body");
        }
        response.body.assign(data.begin() + static_cast<std::ptrdiff_t>(body_start),
                             data.begin() + static_cast<std::ptrdiff_t>(body_start + content_length));
    } else {
        response.body.assign(data.begin() + static_cast<std::ptrdiff_t>(body_start), data.end());
    }

    return response;
}

}  // namespace

ReverseProxy::ReverseProxy(ReverseProxyConfig config)
    : config_(std::move(config)) {
    if (config_.upstream_address.empty()) {
        throw std::invalid_argument("upstream address must not be empty");
    }
    if (config_.upstream_port == 0) {
        throw std::invalid_argument("upstream port must be greater than 0");
    }
    if (config_.connect_timeout.count() <= 0 || config_.io_timeout.count() <= 0) {
        throw std::invalid_argument("proxy timeouts must be greater than 0");
    }
}

http::HttpResponse ReverseProxy::forward(const http::HttpRequest& request) const {
    try {
        net::TCPClient client(net::TCPClientConfig{
            config_.upstream_address,
            config_.upstream_port,
            config_.connect_timeout,
            config_.io_timeout,
        });

        client.connect();
        const std::string serialized = serialize_request(request, config_);
        client.send_all(serialized);

        std::vector<std::uint8_t> response_data;
        for (;;) {
            const auto chunk = client.receive_some();
            if (chunk.empty()) {
                break;
            }
            response_data.insert(response_data.end(), chunk.begin(), chunk.end());

            const std::size_t header_end = find_header_end(response_data);
            if (header_end != std::string::npos) {
                // With Connection: close, receiving EOF gives us the complete body.
                // Keep reading so Content-Length and close-delimited responses both work.
            }
        }

        client.close();
        return parse_response(response_data);
    } catch (const std::exception&) {
        http::HttpResponse response;
        response.status = http::HttpStatus::BadGateway;
        response.set_header("Content-Type", "text/plain");
        const std::string message = "Bad Gateway";
        response.body.assign(message.begin(), message.end());
        return response;
    }
}

const ReverseProxyConfig& ReverseProxy::config() const noexcept {
    return config_;
}

}  // namespace edgex::proxy
