#include "edgex/http/http_parser.hpp"

#include <limits>
#include <utility>

namespace edgex::http {
namespace {

char to_lower_ascii(char ch) {
    if (ch >= 'A' && ch <= 'Z') {
        return static_cast<char>(ch - 'A' + 'a');
    }

    return ch;
}

bool equals_case_insensitive_ascii(std::string_view lhs, std::string_view rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (std::size_t i = 0; i < lhs.size(); ++i) {
        if (to_lower_ascii(lhs[i]) != to_lower_ascii(rhs[i])) {
            return false;
        }
    }

    return true;
}

bool is_tchar(char ch) {
    if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) {
        return true;
    }

    switch (ch) {
    case '!':
    case '#':
    case '$':
    case '%':
    case '&':
    case '\'':
    case '*':
    case '+':
    case '-':
    case '.':
    case '^':
    case '_':
    case '`':
    case '|':
    case '~':
        return true;
    default:
        return false;
    }
}

bool is_visible_field_value_char(char ch) {
    const unsigned char value = static_cast<unsigned char>(ch);
    return ch == '\t' || (value >= 0x20 && value <= 0x7E);
}

std::string_view trim_ows(std::string_view input) {
    std::size_t begin = 0;
    while (begin < input.size() && (input[begin] == ' ' || input[begin] == '\t')) {
        ++begin;
    }

    std::size_t end = input.size();
    while (end > begin && (input[end - 1] == ' ' || input[end - 1] == '\t')) {
        --end;
    }

    return input.substr(begin, end - begin);
}

std::optional<HttpMethod> parse_method(std::string_view token) {
    if (token == "GET") {
        return HttpMethod::Get;
    }

    if (token == "POST") {
        return HttpMethod::Post;
    }

    if (token == "PUT") {
        return HttpMethod::Put;
    }

    if (token == "DELETE") {
        return HttpMethod::Delete_;
    }

    if (token == "HEAD") {
        return HttpMethod::Head;
    }

    if (token == "OPTIONS") {
        return HttpMethod::Options;
    }

    if (token == "PATCH") {
        return HttpMethod::Patch;
    }

    return std::nullopt;
}

std::optional<HttpVersion> parse_version(std::string_view token) {
    if (token == "HTTP/1.0") {
        return HttpVersion::Http10;
    }

    if (token == "HTTP/1.1") {
        return HttpVersion::Http11;
    }

    return std::nullopt;
}

std::optional<std::size_t> parse_content_length_value(std::string_view raw) {
    const std::string_view value = trim_ows(raw);
    if (value.empty()) {
        return std::nullopt;
    }

    std::size_t result = 0;
    constexpr std::size_t kMaxSize = std::numeric_limits<std::size_t>::max();
    for (char ch : value) {
        if (ch < '0' || ch > '9') {
            return std::nullopt;
        }

        const std::size_t digit = static_cast<std::size_t>(ch - '0');
        if (result > (kMaxSize - digit) / 10) {
            return std::nullopt;
        }

        result = (result * 10) + digit;
    }

    return result;
}

}  // namespace

HttpParser::HttpParser(HttpParserLimits limits) : limits_(limits) {}

ParseResult HttpParser::feed(std::string_view chunk) {
    if (state_ == State::Error) {
        return make_error_result();
    }

    if (state_ == State::Complete) {
        if (!chunk.empty()) {
            set_error(ParseErrorCode::UnexpectedDataAfterRequest,
                      "unexpected bytes after complete request", 0);
            return make_error_result();
        }

        return make_complete_result();
    }

    if (!chunk.empty()) {
        buffer_.append(chunk.data(), chunk.size());
    }

    return parse_available_data();
}

void HttpParser::reset() noexcept {
    state_ = State::RequestLine;
    buffer_.clear();
    expected_body_bytes_ = 0;
    content_length_present_ = false;
    completed_request_ = std::nullopt;
    last_error_ = std::nullopt;
}

bool HttpParser::has_error() const noexcept {
    return state_ == State::Error;
}

bool HttpParser::is_waiting_for_more() const noexcept {
    return state_ == State::RequestLine || state_ == State::Headers || state_ == State::Body;
}

std::size_t HttpParser::buffered_bytes() const noexcept {
    return buffer_.size();
}

ParseResult HttpParser::parse_available_data() {
    while (true) {
        if (state_ == State::RequestLine) {
            const std::size_t line_end = buffer_.find("\r\n");
            if (line_end == std::string::npos) {
                if (buffer_.size() > limits_.max_start_line_bytes) {
                    set_error(ParseErrorCode::StartLineTooLarge,
                              "request line exceeds max_start_line_bytes", buffer_.size());
                    return make_error_result();
                }

                return make_incomplete_result();
            }

            if (line_end > limits_.max_start_line_bytes) {
                set_error(ParseErrorCode::StartLineTooLarge,
                          "request line exceeds max_start_line_bytes", line_end);
                return make_error_result();
            }

            if (!parse_request_line(line_end)) {
                return make_error_result();
            }

            buffer_.erase(0, line_end + 2);
            state_ = State::Headers;
            continue;
        }

        if (state_ == State::Headers) {
            if (buffer_.size() >= 2 && buffer_.compare(0, 2, "\r\n") == 0) {
                if (!parse_headers(0)) {
                    return make_error_result();
                }

                if (!prepare_body_expectation()) {
                    return make_error_result();
                }

                buffer_.erase(0, 2);

                if (expected_body_bytes_ == 0) {
                    if (content_length_present_) {
                        working_request_.body = std::vector<std::uint8_t>{};
                    }

                    if (!buffer_.empty()) {
                        set_error(ParseErrorCode::UnexpectedDataAfterRequest,
                                  "unexpected bytes after complete request", 0);
                        return make_error_result();
                    }

                    completed_request_ = working_request_;
                    state_ = State::Complete;
                    return make_complete_result();
                }

                state_ = State::Body;
                continue;
            }

            const std::size_t headers_end = buffer_.find("\r\n\r\n");
            if (headers_end == std::string::npos) {
                if (buffer_.size() > limits_.max_header_bytes) {
                    set_error(ParseErrorCode::HeaderTooLarge,
                              "headers exceed max_header_bytes", buffer_.size());
                    return make_error_result();
                }

                return make_incomplete_result();
            }

            if (headers_end > limits_.max_header_bytes) {
                set_error(ParseErrorCode::HeaderTooLarge,
                          "headers exceed max_header_bytes", headers_end);
                return make_error_result();
            }

            if (!parse_headers(headers_end)) {
                return make_error_result();
            }

            if (!prepare_body_expectation()) {
                return make_error_result();
            }

            const std::size_t body_start = headers_end + 4;
            if (expected_body_bytes_ == 0) {
                if (content_length_present_) {
                    working_request_.body = std::vector<std::uint8_t>{};
                }

                if (buffer_.size() > body_start) {
                    set_error(ParseErrorCode::UnexpectedDataAfterRequest,
                              "unexpected bytes after complete request", body_start);
                    return make_error_result();
                }

                completed_request_ = working_request_;
                state_ = State::Complete;
                return make_complete_result();
            }

            // Body expected: erase headers, keep only body bytes in buffer, continue to Body state
            buffer_.erase(0, body_start);
            state_ = State::Body;
            continue;
        }

        if (state_ == State::Body) {
            // buffer_ contains ONLY body bytes
            if (buffer_.size() < expected_body_bytes_) {
                // Still need more bytes
                return make_incomplete_result();
            }

            if (buffer_.size() > expected_body_bytes_) {
                // Extra bytes after expected body
                set_error(ParseErrorCode::UnexpectedDataAfterRequest,
                          "unexpected bytes after complete request body", expected_body_bytes_);
                return make_error_result();
            }

            // buffer_.size() == expected_body_bytes_: extract and assign body
            std::vector<std::uint8_t> body_bytes;
            body_bytes.reserve(expected_body_bytes_);
            for (std::size_t i = 0; i < expected_body_bytes_; ++i) {
                body_bytes.push_back(static_cast<std::uint8_t>(buffer_[i]));
            }
            
            working_request_.body = std::move(body_bytes);
            buffer_.clear();

            completed_request_ = working_request_;
            state_ = State::Complete;
            return make_complete_result();
        }

        if (state_ == State::Complete) {
            return make_complete_result();
        }

        return make_error_result();
    }
}

bool HttpParser::parse_request_line(std::size_t line_end) {
    const std::string_view line(buffer_.data(), line_end);

    const std::size_t first_space = line.find(' ');
    if (first_space == std::string_view::npos || first_space == 0) {
        set_error(ParseErrorCode::InvalidRequestLine, "invalid request line format", 0);
        return false;
    }

    const std::size_t second_space = line.find(' ', first_space + 1);
    if (second_space == std::string_view::npos || second_space == first_space + 1) {
        set_error(ParseErrorCode::InvalidRequestLine, "invalid request line format", 0);
        return false;
    }

    if (line.find(' ', second_space + 1) != std::string_view::npos) {
        set_error(ParseErrorCode::InvalidRequestLine,
                  "request line must contain exactly 3 tokens", second_space + 1);
        return false;
    }

    const std::string_view method_token = line.substr(0, first_space);
    const std::string_view target_token =
        line.substr(first_space + 1, second_space - first_space - 1);
    const std::string_view version_token = line.substr(second_space + 1);

    const std::optional<HttpMethod> method = parse_method(method_token);
    if (!method) {
        set_error(ParseErrorCode::InvalidMethod, "unsupported HTTP method", 0);
        return false;
    }

    if (target_token.empty() || target_token.front() != '/') {
        set_error(ParseErrorCode::InvalidTarget, "request target must begin with '/'", first_space + 1);
        return false;
    }

    const std::optional<HttpVersion> version = parse_version(version_token);
    if (!version) {
        set_error(ParseErrorCode::InvalidVersion, "unsupported HTTP version", second_space + 1);
        return false;
    }

    working_request_ = HttpRequest{};
    working_request_.method = *method;
    working_request_.target.assign(target_token.data(), target_token.size());
    working_request_.version = *version;
    working_request_.headers.clear();
    working_request_.body = std::nullopt;

    return true;
}

bool HttpParser::parse_headers(std::size_t headers_end) {
    std::size_t current = 0;
    while (current < headers_end) {
        const std::size_t line_start = current;
        const std::size_t next = buffer_.find("\r\n", current);
        if (next == std::string::npos || next > headers_end) {
            set_error(ParseErrorCode::InvalidHeaderLine, "malformed header line terminator",
                      line_start);
            return false;
        }

        const std::string_view line(buffer_.data() + current, next - current);
        current = next + 2;

        if (line.empty()) {
            set_error(ParseErrorCode::InvalidHeaderLine, "empty header line inside header block",
                      line_start);
            return false;
        }

        if (line.front() == ' ' || line.front() == '\t') {
            set_error(ParseErrorCode::InvalidHeaderLine,
                      "obs-fold header continuation is not supported", line_start);
            return false;
        }

        const std::size_t colon = line.find(':');
        if (colon == std::string_view::npos || colon == 0) {
            set_error(ParseErrorCode::InvalidHeaderLine, "header must contain name:value",
                      line_start);
            return false;
        }

        const std::string_view name = line.substr(0, colon);
        for (std::size_t index = 0; index < name.size(); ++index) {
            const char ch = name[index];
            if (!is_tchar(ch)) {
                set_error(ParseErrorCode::InvalidHeaderLine, "invalid header name character",
                          line_start + index);
                return false;
            }
        }

        const std::string_view value = line.substr(colon + 1);
        for (std::size_t index = 0; index < value.size(); ++index) {
            const char ch = value[index];
            if (!is_visible_field_value_char(ch)) {
                set_error(ParseErrorCode::InvalidHeaderLine, "invalid header value character",
                          line_start + colon + 1 + index);
                return false;
            }
        }

        HeaderField field;
        field.name.assign(name.data(), name.size());
        field.value.assign(value.data(), value.size());
        working_request_.headers.push_back(std::move(field));
    }

    return true;
}

bool HttpParser::prepare_body_expectation() {
    content_length_present_ = false;
    std::optional<std::size_t> declared_content_length;
    std::size_t header_search_start = 0;
    std::size_t content_length_value_offset = 0;

    for (const HeaderField& header : working_request_.headers) {
        const std::size_t header_offset = buffer_.find(header.name, header_search_start);
        const std::size_t value_offset = header_offset + header.name.size() + 1;
        const std::size_t line_end = buffer_.find("\r\n", value_offset);
        header_search_start = line_end == std::string::npos ? value_offset : line_end + 2;

        if (equals_case_insensitive_ascii(header.name, "Transfer-Encoding")) {
            set_error(ParseErrorCode::UnsupportedTransferEncoding,
                      "Transfer-Encoding is not supported", header_offset);
            return false;
        }

        if (equals_case_insensitive_ascii(header.name, "Content-Length")) {
            content_length_present_ = true;
            const std::optional<std::size_t> parsed = parse_content_length_value(header.value);
            if (!parsed) {
                set_error(ParseErrorCode::InvalidContentLength, "invalid Content-Length value",
                          value_offset);
                return false;
            }

            if (declared_content_length && declared_content_length.value() != parsed.value()) {
                set_error(ParseErrorCode::InvalidContentLength,
                          "conflicting Content-Length headers", header_offset);
                return false;
            }

            declared_content_length = parsed;
            content_length_value_offset = value_offset;
        }
    }

    expected_body_bytes_ = declared_content_length.value_or(0);
    if (expected_body_bytes_ > limits_.max_body_bytes) {
        set_error(ParseErrorCode::BodyTooLarge, "body exceeds max_body_bytes",
                  content_length_value_offset);
        return false;
    }

    return true;
}

bool HttpParser::finalize_body_if_ready() {
    if (buffer_.size() < expected_body_bytes_) {
        if (buffer_.size() > limits_.max_body_bytes) {
            set_error(ParseErrorCode::BodyTooLarge, "body exceeds max_body_bytes", buffer_.size());
            return false;
        }

        return false;
    }

    if (buffer_.size() > expected_body_bytes_) {
        set_error(ParseErrorCode::UnexpectedDataAfterRequest,
                  "unexpected bytes after complete request body", expected_body_bytes_);
        return false;
    }

    std::vector<std::uint8_t> body_bytes;
    body_bytes.reserve(expected_body_bytes_);
    for (std::size_t i = 0; i < expected_body_bytes_; ++i) {
        body_bytes.push_back(static_cast<std::uint8_t>(buffer_[i]));
    }

    working_request_.body = std::move(body_bytes);
    buffer_.erase(0, expected_body_bytes_);
    return true;
}

void HttpParser::set_error(ParseErrorCode code, std::string message, std::size_t offset) {
    state_ = State::Error;
    last_error_ = ParseError{code, std::move(message), offset};
}

ParseResult HttpParser::make_incomplete_result() const {
    return ParseResult{ParseStatus::Incomplete, std::nullopt, std::nullopt};
}

ParseResult HttpParser::make_complete_result() const {
    return ParseResult{ParseStatus::Complete, completed_request_, std::nullopt};
}

ParseResult HttpParser::make_error_result() const {
    return ParseResult{ParseStatus::Error, std::nullopt, last_error_};
}

}  // namespace edgex::http
