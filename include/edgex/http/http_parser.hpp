#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "edgex/http/http_request.hpp"

namespace edgex::http {

enum class ParseStatus {
    Incomplete,
    Complete,
    Error,
};

enum class ParseErrorCode {
    InvalidRequestLine,
    InvalidMethod,
    InvalidTarget,
    InvalidVersion,
    StartLineTooLarge,
    InvalidHeaderLine,
    HeaderTooLarge,
    InvalidContentLength,
    BodyTooLarge,
    UnsupportedTransferEncoding,
    UnexpectedDataAfterRequest,
};

struct ParseError {
    ParseErrorCode code{};
    std::string message;
    std::size_t offset{0};
};

struct ParseResult {
    ParseStatus status{ParseStatus::Incomplete};
    std::optional<HttpRequest> request;
    std::optional<ParseError> error;
};

struct HttpParserLimits {
    std::size_t max_start_line_bytes{8 * 1024};
    std::size_t max_header_bytes{64 * 1024};
    std::size_t max_body_bytes{1024 * 1024};
};

class HttpParser {
public:
    explicit HttpParser(HttpParserLimits limits = {});

    ParseResult feed(std::string_view chunk);

    void reset() noexcept;

    [[nodiscard]] bool has_error() const noexcept;
    [[nodiscard]] bool is_waiting_for_more() const noexcept;
    [[nodiscard]] std::size_t buffered_bytes() const noexcept;

private:
    enum class State {
        RequestLine,
        Headers,
        Body,
        Complete,
        Error,
    };

    ParseResult parse_available_data();

    bool parse_request_line(std::size_t line_end);
    bool parse_headers(std::size_t headers_end);
    bool prepare_body_expectation();
    bool finalize_body_if_ready();

    void set_error(ParseErrorCode code, std::string message, std::size_t offset);

    ParseResult make_incomplete_result() const;
    ParseResult make_complete_result() const;
    ParseResult make_error_result() const;

    HttpParserLimits limits_{};
    State state_{State::RequestLine};
    std::string buffer_;
    std::size_t expected_body_bytes_{0};
    bool content_length_present_{false};
    HttpRequest working_request_{};
    std::optional<HttpRequest> completed_request_;
    std::optional<ParseError> last_error_;
};

}  // namespace edgex::http
