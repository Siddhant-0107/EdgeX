#include "edgex/http/http_response.hpp"
#include "edgex/static/static_file_server.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <utility>

using namespace edgex::http;
using edgex::static_files::StaticFileServer;

namespace {

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

HttpRequest make_request(HttpMethod method, std::string target) {
    HttpRequest request;
    request.method = method;
    request.target = std::move(target);
    request.version = HttpVersion::Http11;
    return request;
}

std::string body_as_string(const HttpResponse& response) {
    return std::string(response.body.begin(), response.body.end());
}

std::filesystem::path fixture_root() {
    return std::filesystem::path(EDGEX_SOURCE_DIR) /
           "tests" /
           "fixtures" /
           "static";
}

void test_existing_html_file() {
    StaticFileServer server(fixture_root());

    const auto response =
        server.serve(make_request(HttpMethod::Get, "/index.html"));

    expect(response.status == HttpStatus::Ok,
           "existing HTML file should return 200");

    expect(response.has_header("Content-Type"),
           "HTML response should have Content-Type");

    expect(body_as_string(response) ==
               "<html><body>Hello EdgeX</body></html>\n",
           "HTML file contents should be returned");

    std::cout << "PASS: existing HTML file\n";
}

void test_text_file() {
    StaticFileServer server(fixture_root());

    const auto response =
        server.serve(make_request(HttpMethod::Get, "/test.txt"));

    expect(response.status == HttpStatus::Ok,
           "existing text file should return 200");

    expect(body_as_string(response) ==
               "EdgeX static file test\n",
           "text file contents should be returned");

    std::cout << "PASS: text file\n";
}

void test_mime_types() {
    StaticFileServer server(fixture_root());

    const auto html =
        server.serve(
            make_request(HttpMethod::Get, "/index.html"));

    const auto css =
        server.serve(
            make_request(HttpMethod::Get, "/style.css"));

    expect(html.has_header("Content-Type"),
           "HTML should have Content-Type");

    expect(css.has_header("Content-Type"),
           "CSS should have Content-Type");

    std::cout << "PASS: MIME types\n";
}

void test_missing_file_returns_404() {
    StaticFileServer server(fixture_root());

    const auto response =
        server.serve(
            make_request(HttpMethod::Get, "/missing.txt"));

    expect(response.status == HttpStatus::NotFound,
           "missing file should return 404");

    expect(body_as_string(response) == "Not Found",
           "missing file should return Not Found");

    std::cout << "PASS: missing file returns 404\n";
}

void test_method_not_allowed() {
    StaticFileServer server(fixture_root());

    const auto response =
        server.serve(
            make_request(HttpMethod::Post, "/index.html"));

    expect(response.status == HttpStatus::MethodNotAllowed,
           "non-GET request should return 405");

    std::cout << "PASS: method not allowed\n";
}

void test_query_string_is_ignored() {
    StaticFileServer server(fixture_root());

    const auto response =
        server.serve(
            make_request(
                HttpMethod::Get,
                "/index.html?version=1"));

    expect(response.status == HttpStatus::Ok,
           "query string should not prevent file lookup");

    expect(body_as_string(response) ==
               "<html><body>Hello EdgeX</body></html>\n",
           "query string should be excluded from filesystem path");

    std::cout << "PASS: query string handling\n";
}

void test_path_traversal_is_rejected() {
    StaticFileServer server(fixture_root());

    const auto response =
        server.serve(
            make_request(
                HttpMethod::Get,
                "/../secret.txt"));

    expect(response.status == HttpStatus::NotFound,
           "path traversal should return 404");

    expect(body_as_string(response) == "Not Found",
           "path traversal should not expose file contents");

    std::cout << "PASS: path traversal protection\n";
}

void test_nested_path_traversal_is_rejected() {
    StaticFileServer server(fixture_root());

    const auto response =
        server.serve(
            make_request(
                HttpMethod::Get,
                "/subdir/../../secret.txt"));

    expect(response.status == HttpStatus::NotFound,
           "nested path traversal should return 404");

    std::cout << "PASS: nested path traversal protection\n";
}

} // namespace

int main() {
    test_existing_html_file();
    test_text_file();
    test_mime_types();
    test_missing_file_returns_404();
    test_method_not_allowed();
    test_query_string_is_ignored();
    test_path_traversal_is_rejected();
    test_nested_path_traversal_is_rejected();

    std::cout << "All 8 static file server tests passed\n";

    return EXIT_SUCCESS;
}
