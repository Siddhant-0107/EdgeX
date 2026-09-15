#include "edgex/http/http_response.hpp"
#include "edgex/http/http_response_builder.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

using namespace edgex::http;

namespace {

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void test_default_response() {
    HttpResponse response;

    expect(response.status == HttpStatus::Ok,
           "default status should be 200 OK");

    expect(response.headers.empty(),
           "default response should have no headers");

    expect(response.body.empty(),
           "default response should have an empty body");

    std::cout << "PASS: default response\n";
}

void test_set_header() {
    HttpResponse response;

    response.set_header("Content-Type", "text/plain");

    expect(response.headers.size() == 1,
           "set_header should add a header");

    expect(response.headers[0].name == "Content-Type",
           "header name should be preserved");

    expect(response.headers[0].value == "text/plain",
           "header value should be preserved");

    std::cout << "PASS: set header\n";
}

void test_header_replacement_is_case_insensitive() {
    HttpResponse response;

    response.set_header("Content-Type", "text/plain");
    response.set_header("content-type", "application/json");

    expect(response.headers.size() == 1,
           "same header with different casing should replace");

    expect(response.headers[0].value == "application/json",
           "replaced header should contain the new value");

    std::cout << "PASS: case-insensitive header replacement\n";
}

void test_has_header_is_case_insensitive() {
    HttpResponse response;

    response.set_header("Content-Type", "text/plain");

    expect(response.has_header("Content-Type"),
           "exact header lookup should succeed");

    expect(response.has_header("content-type"),
           "lowercase header lookup should succeed");

    expect(response.has_header("CONTENT-TYPE"),
           "uppercase header lookup should succeed");

    expect(!response.has_header("Content-Length"),
           "missing header lookup should fail");

    std::cout << "PASS: case-insensitive header lookup\n";
}

void test_builder_status() {
    const auto response =
        HttpResponseBuilder{}
            .status(HttpStatus::NotFound)
            .build();

    expect(response.status == HttpStatus::NotFound,
           "builder should set status");

    std::cout << "PASS: builder status\n";
}

void test_builder_header() {
    const auto response =
        HttpResponseBuilder{}
            .header("Content-Type", "text/plain")
            .build();

    expect(response.has_header("Content-Type"),
           "builder should add headers");

    expect(response.headers.size() == 1,
           "builder should add exactly one header");

    expect(response.headers[0].value == "text/plain",
           "builder should preserve header value");

    std::cout << "PASS: builder header\n";
}

void test_builder_string_body() {
    const auto response =
        HttpResponseBuilder{}
            .body("Hello")
            .build();

    expect(response.body.size() == 5,
           "string body should contain five bytes");

    const std::string body(response.body.begin(), response.body.end());

    expect(body == "Hello",
           "string body should preserve its contents");

    std::cout << "PASS: builder string body\n";
}

void test_builder_byte_body() {
    const std::vector<std::uint8_t> expected{
        0x01, 0x02, 0x03, 0xFF
    };

    const auto response =
        HttpResponseBuilder{}
            .body(expected)
            .build();

    expect(response.body == expected,
           "byte body should preserve all bytes");

    std::cout << "PASS: builder byte body\n";
}

void test_builder_chaining() {
    const auto response =
        HttpResponseBuilder{}
            .status(HttpStatus::Created)
            .header("Content-Type", "application/json")
            .body("{\"ok\":true}")
            .build();

    expect(response.status == HttpStatus::Created,
           "chained builder should set status");

    expect(response.has_header("content-type"),
           "chained builder should set header");

    const std::string body(response.body.begin(), response.body.end());

    expect(body == "{\"ok\":true}",
           "chained builder should set body");

    std::cout << "PASS: builder chaining\n";
}

} // namespace
void test_serialize_basic_response() {
    const auto response =
        HttpResponseBuilder{}
            .status(HttpStatus::Ok)
            .header("Content-Type", "text/plain")
            .body("Hello")
            .build();

    const std::string serialized = response.serialize();

    const std::string expected =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "Hello";

    expect(serialized == expected,
           "basic response serialization should match expected output");

    std::cout << "PASS: basic response serialization\n";
}

void test_serialize_empty_body() {
    const auto response =
        HttpResponseBuilder{}
            .status(HttpStatus::NoContent)
            .build();

    const std::string serialized = response.serialize();

    const std::string expected =
        "HTTP/1.1 204 No Content\r\n"
        "Content-Length: 0\r\n"
        "\r\n";

    expect(serialized == expected,
           "empty response serialization should contain zero Content-Length");

    std::cout << "PASS: empty body serialization\n";
}

void test_serialize_custom_content_length() {
    const auto response =
        HttpResponseBuilder{}
            .status(HttpStatus::Ok)
            .header("Content-Length", "5")
            .body("Hello")
            .build();

    const std::string serialized = response.serialize();

    const std::string expected =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "Hello";

    expect(serialized == expected,
           "explicit Content-Length should not be duplicated");

    std::cout << "PASS: custom Content-Length serialization\n";
}

void test_serialize_multiple_headers() {
    const auto response =
        HttpResponseBuilder{}
            .status(HttpStatus::Created)
            .header("Content-Type", "application/json")
            .header("Connection", "close")
            .body("{\"ok\":true}")
            .build();

    const std::string serialized = response.serialize();

    const std::string expected =
        "HTTP/1.1 201 Created\r\n"
        "Content-Type: application/json\r\n"
        "Connection: close\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "{\"ok\":true}";

    expect(serialized == expected,
           "multiple headers should serialize correctly");

    std::cout << "PASS: multiple header serialization\n";
}
int main() {
    test_default_response();
    test_set_header();
    test_header_replacement_is_case_insensitive();
    test_has_header_is_case_insensitive();
    test_builder_status();
    test_builder_header();
    test_builder_string_body();
    test_builder_byte_body();
    test_builder_chaining();
    test_serialize_basic_response();
test_serialize_empty_body();
test_serialize_custom_content_length();
test_serialize_multiple_headers();
    std::cout << "All 13 response tests passed\n";

    return EXIT_SUCCESS;
}
