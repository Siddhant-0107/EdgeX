#include "edgex/http/http_parser.hpp"

#include <cstdint>
#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using edgex::http::HttpMethod;
using edgex::http::HttpParser;
using edgex::http::HttpParserLimits;
using edgex::http::HttpVersion;
using edgex::http::ParseErrorCode;
using edgex::http::ParseResult;
using edgex::http::ParseStatus;

[[noreturn]] void fail(const std::string& message) {
    throw std::runtime_error(message);
}

void expect(bool condition, const std::string& message) {
    if (!condition) {
        fail(message);
    }
}

edgex::http::HttpRequest expect_complete(const ParseResult& result,
                                                const std::string& context) {
    expect(result.status == ParseStatus::Complete, context + ": expected Complete");
    expect(result.request.has_value(), context + ": expected a request");
    expect(!result.error.has_value(), context + ": did not expect an error");
    return *result.request;
}

void expect_incomplete(const ParseResult& result, const std::string& context) {
    expect(result.status == ParseStatus::Incomplete, context + ": expected Incomplete");
    expect(!result.request.has_value(), context + ": did not expect a request");
    expect(!result.error.has_value(), context + ": did not expect an error");
}

void expect_error(const ParseResult& result, ParseErrorCode code, const std::string& context) {
    expect(result.status == ParseStatus::Error, context + ": expected Error");
    expect(result.error.has_value(), context + ": expected an error payload");
    expect(result.error->code == code, context + ": unexpected error code");
    expect(!result.request.has_value(), context + ": did not expect a request");
}

std::string body_as_string(const edgex::http::HttpRequest& request) {
    expect(request.body.has_value(), "expected request body to be present");
    return std::string(request.body->begin(), request.body->end());
}

void test_basic_get() {
    HttpParser parser;
    const auto& request = expect_complete(
        parser.feed("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"), "basic GET");

    expect(request.method == HttpMethod::Get, "basic GET: wrong method");
    expect(request.target == "/", "basic GET: wrong target");
    expect(request.version == HttpVersion::Http11, "basic GET: wrong version");
    const auto host = request.header_value("Host");
    expect(host.has_value() && *host == "localhost", "basic GET: Host not retrievable");
}

void test_headerless_get() {
    HttpParser parser;
    const auto& request = expect_complete(parser.feed("GET / HTTP/1.1\r\n\r\n"),
                                          "headerless GET");

    expect(request.headers.empty(), "headerless GET: expected no headers");
    expect(!request.body.has_value(), "headerless GET: expected no body");
}

void test_request_line_split_across_feeds() {
    HttpParser parser;
    expect_incomplete(parser.feed("GET /hel"), "split request line first feed");
    const auto& request = expect_complete(
        parser.feed("lo HTTP/1.1\r\nHost: localhost\r\n\r\n"), "split request line final feed");

    expect(request.target == "/hello", "split request line: wrong target");
}

void test_headers_split_across_feeds() {
    HttpParser parser;
    expect_incomplete(parser.feed("GET / HTTP/1.1\r\nHost: loca"),
                      "split headers first feed");
    expect_incomplete(parser.feed("lhost\r\nX-Trace: abc"), "split headers second feed");
    const auto& request = expect_complete(parser.feed("\r\n\r\n"), "split headers final feed");

    expect(request.headers.size() == 2, "split headers: expected two headers");
}

void test_body_split_across_feeds() {
    HttpParser parser;
    expect_incomplete(parser.feed("POST /data HTTP/1.1\r\nContent-Length: 5\r\n\r\nhe"),
                      "split body first feed");
    const auto& request = expect_complete(parser.feed("llo"), "split body final feed");

    expect(request.method == HttpMethod::Post, "split body: wrong method");
    expect(body_as_string(request) == "hello", "split body: wrong body");
}

void test_case_insensitive_header_lookup() {
    HttpParser parser;
    const auto& request = expect_complete(
        parser.feed("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"), "header lookup");

    for (const std::string_view name : {"host", "HOST", "HoSt"}) {
        const auto host = request.header_value(name);
        expect(host.has_value() && *host == "localhost", "header lookup: case-insensitive miss");
        expect(request.has_header(name), "header lookup: has_header case-insensitive miss");
    }
}

void test_duplicate_headers_are_preserved() {
    HttpParser parser;
    const auto& request = expect_complete(
        parser.feed("GET / HTTP/1.1\r\nX-Tag: first\r\nX-Tag: second\r\n\r\n"),
        "duplicate headers");

    expect(request.headers.size() == 2, "duplicate headers: expected both fields");
    expect(request.headers[0].name == "X-Tag" && request.headers[0].value == " first",
           "duplicate headers: first field not preserved");
    expect(request.headers[1].name == "X-Tag" && request.headers[1].value == " second",
           "duplicate headers: second field not preserved");
}

void test_invalid_request_line() {
    HttpParser parser;
    expect_error(parser.feed("GET / HTTP/1.1 extra\r\n\r\n"), ParseErrorCode::InvalidRequestLine,
                 "invalid request line");
}

void test_invalid_http_version() {
    HttpParser parser;
    expect_error(parser.feed("GET / HTTP/2.0\r\n\r\n"), ParseErrorCode::InvalidVersion,
                 "invalid HTTP version");
}

void test_invalid_header_syntax() {
    HttpParser parser;
    expect_error(parser.feed("GET / HTTP/1.1\r\nHost localhost\r\n\r\n"),
                 ParseErrorCode::InvalidHeaderLine, "invalid header syntax");
}

void test_invalid_content_length_values() {
    for (const std::string_view value : {"five", "5x", "184467440737095516161"}) {
        HttpParser parser;
        const std::string request = "POST / HTTP/1.1\r\nContent-Length: " +
                                    std::string(value) + "\r\n\r\n";
        expect_error(parser.feed(request), ParseErrorCode::InvalidContentLength,
                     "invalid Content-Length value");
    }
}

void test_conflicting_content_length() {
    HttpParser parser;
    expect_error(parser.feed("POST / HTTP/1.1\r\nContent-Length: 3\r\nContent-Length: 4\r\n\r\n"),
                 ParseErrorCode::InvalidContentLength, "conflicting Content-Length");
}

void test_body_size_limit() {
    HttpParser parser(HttpParserLimits{8 * 1024, 64 * 1024, 4});
    expect_error(parser.feed("POST / HTTP/1.1\r\nContent-Length: 5\r\n\r\n"),
                 ParseErrorCode::BodyTooLarge, "body size limit");
}

void test_transfer_encoding_is_rejected() {
    for (const std::string_view value : {"chunked", "gzip", "custom-coding"}) {
        HttpParser parser;
        const std::string request = "POST / HTTP/1.1\r\nTransfer-Encoding: " +
                                    std::string(value) + "\r\n\r\n";
        expect_error(parser.feed(request), ParseErrorCode::UnsupportedTransferEncoding,
                     "Transfer-Encoding rejection");
    }
}

void test_complete_state_behavior() {
    HttpParser parser;
    expect_complete(parser.feed("GET / HTTP/1.1\r\n\r\n"), "complete state initial request");
    expect_complete(parser.feed(""), "complete state empty feed");
    expect_error(parser.feed("extra"), ParseErrorCode::UnexpectedDataAfterRequest,
                 "complete state non-empty feed");
}

void test_error_state_behavior() {
    HttpParser parser;
    expect_error(parser.feed("GET / HTTP/2.0\r\n\r\n"), ParseErrorCode::InvalidVersion,
                 "error state initial failure");
    expect_error(parser.feed("GET / HTTP/1.1\r\n\r\n"), ParseErrorCode::InvalidVersion,
                 "error state remains sticky");

    parser.reset();
    expect(!parser.has_error(), "error state reset: parser still reports an error");
    expect_incomplete(parser.feed("GET /"), "error state reset: parser did not restart");
}

void test_reset_after_complete() {
    HttpParser parser;
    expect_complete(parser.feed("GET /first HTTP/1.1\r\n\r\n"), "reset complete first request");

    parser.reset();
    const auto& request = expect_complete(parser.feed("GET /second HTTP/1.1\r\n\r\n"),
                                          "reset complete second request");
    expect(request.target == "/second", "reset complete: wrong second request");
}

void test_header_body_boundary() {
    HttpParser parser;
    const auto& request = expect_complete(
        parser.feed("POST /data HTTP/1.1\r\nX-Mode: test\r\nContent-Length: 3\r\n\r\nabc"),
        "header/body boundary");

    expect(request.headers.size() == 2, "header/body boundary: body was treated as a header");
    expect(body_as_string(request) == "abc", "header/body boundary: wrong body");
}

struct TestCase {
    const char* name;
    std::function<void()> run;
};

}  // namespace

int main() {
    const std::vector<TestCase> tests{
        {"basic GET", test_basic_get},
        {"headerless GET regression", test_headerless_get},
        {"request line split across feeds", test_request_line_split_across_feeds},
        {"headers split across feeds", test_headers_split_across_feeds},
        {"body split across feeds", test_body_split_across_feeds},
        {"case-insensitive header lookup", test_case_insensitive_header_lookup},
        {"duplicate headers", test_duplicate_headers_are_preserved},
        {"invalid request line", test_invalid_request_line},
        {"invalid HTTP version", test_invalid_http_version},
        {"invalid header syntax", test_invalid_header_syntax},
        {"invalid Content-Length values", test_invalid_content_length_values},
        {"conflicting Content-Length", test_conflicting_content_length},
        {"body size limit", test_body_size_limit},
        {"Transfer-Encoding rejection", test_transfer_encoding_is_rejected},
        {"complete-state behavior", test_complete_state_behavior},
        {"error-state behavior", test_error_state_behavior},
        {"reset after complete", test_reset_after_complete},
        {"header/body boundary", test_header_body_boundary},
    };

    std::size_t failures = 0;
    for (const TestCase& test : tests) {
        try {
            test.run();
            std::cout << "PASS: " << test.name << '\n';
        } catch (const std::exception& ex) {
            ++failures;
            std::cerr << "FAIL: " << test.name << ": " << ex.what() << '\n';
        }
    }

    if (failures != 0) {
        std::cerr << failures << " parser test(s) failed\n";
        return 1;
    }

    std::cout << "All " << tests.size() << " parser tests passed\n";
    return 0;
}

