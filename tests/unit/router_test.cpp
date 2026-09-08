#include "edgex/http/router.hpp"
#include "edgex/http/http_response_builder.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>

using namespace edgex::http;

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

void test_get_route_matches() {
    Router router;

    router.get("/hello", [](const HttpRequest&) {
        return HttpResponseBuilder{}
            .status(HttpStatus::Ok)
            .body("Hello")
            .build();
    });

    const auto response =
        router.handle(make_request(HttpMethod::Get, "/hello"));

    expect(response.status == HttpStatus::Ok,
           "GET route should return 200");

    expect(body_as_string(response) == "Hello",
           "GET route should execute its handler");

    std::cout << "PASS: GET route matching\n";
}

void test_post_route_matches() {
    Router router;

    router.post("/users", [](const HttpRequest&) {
        return HttpResponseBuilder{}
            .status(HttpStatus::Created)
            .body("Created")
            .build();
    });

    const auto response =
        router.handle(make_request(HttpMethod::Post, "/users"));

    expect(response.status == HttpStatus::Created,
           "POST route should return 201");

    expect(body_as_string(response) == "Created",
           "POST route should execute its handler");

    std::cout << "PASS: POST route matching\n";
}

void test_method_must_match() {
    Router router;

    router.get("/hello", [](const HttpRequest&) {
        return HttpResponseBuilder{}
            .status(HttpStatus::Ok)
            .body("GET")
            .build();
    });

    const auto response =
        router.handle(make_request(HttpMethod::Post, "/hello"));

    expect(response.status == HttpStatus::NotFound,
           "wrong method should not match the route");

    std::cout << "PASS: method matching\n";
}

void test_path_must_match() {
    Router router;

    router.get("/hello", [](const HttpRequest&) {
        return HttpResponseBuilder{}
            .status(HttpStatus::Ok)
            .body("Hello")
            .build();
    });

    const auto response =
        router.handle(make_request(HttpMethod::Get, "/goodbye"));

    expect(response.status == HttpStatus::NotFound,
           "wrong path should not match the route");

    std::cout << "PASS: path matching\n";
}

void test_unmatched_route_returns_404() {
    Router router;

    const auto response =
        router.handle(make_request(HttpMethod::Get, "/missing"));

    expect(response.status == HttpStatus::NotFound,
           "unmatched route should return 404");

    expect(body_as_string(response) == "Not Found",
           "unmatched route should return Not Found body");

    std::cout << "PASS: unmatched route returns 404\n";
}

void test_multiple_routes() {
    Router router;

    router.get("/one", [](const HttpRequest&) {
        return HttpResponseBuilder{}
            .status(HttpStatus::Ok)
            .body("one")
            .build();
    });

    router.get("/two", [](const HttpRequest&) {
        return HttpResponseBuilder{}
            .status(HttpStatus::Ok)
            .body("two")
            .build();
    });

    const auto first =
        router.handle(make_request(HttpMethod::Get, "/one"));

    const auto second =
        router.handle(make_request(HttpMethod::Get, "/two"));

    expect(body_as_string(first) == "one",
           "first route should execute correctly");

    expect(body_as_string(second) == "two",
           "second route should execute correctly");

    std::cout << "PASS: multiple routes\n";
}

void test_first_matching_route_wins() {
    Router router;

    router.get("/hello", [](const HttpRequest&) {
        return HttpResponseBuilder{}
            .status(HttpStatus::Ok)
            .body("first")
            .build();
    });

    router.get("/hello", [](const HttpRequest&) {
        return HttpResponseBuilder{}
            .status(HttpStatus::Created)
            .body("second")
            .build();
    });

    const auto response =
        router.handle(make_request(HttpMethod::Get, "/hello"));

    expect(response.status == HttpStatus::Ok,
           "first matching route should win");

    expect(body_as_string(response) == "first",
           "route conflict should be deterministic");

    std::cout << "PASS: deterministic route conflict\n";
}

} // namespace

int main() {
    test_get_route_matches();
    test_post_route_matches();
    test_method_must_match();
    test_path_must_match();
    test_unmatched_route_returns_404();
    test_multiple_routes();
    test_first_matching_route_wins();

    std::cout << "All 7 router tests passed\n";

    return EXIT_SUCCESS;
}
