#include "edgex/http/router.hpp"
#include "edgex/http/http_response_builder.hpp"

#include <utility>
namespace edgex::http {

void Router::add_route(HttpMethod method,
                       std::string path,
                       HttpHandler handler) {
    routes_.push_back(
        Route{method, std::move(path), std::move(handler)});
}

void Router::get(std::string path, HttpHandler handler) {
    add_route(HttpMethod::Get, std::move(path), std::move(handler));
}

void Router::post(std::string path, HttpHandler handler) {
    add_route(HttpMethod::Post, std::move(path), std::move(handler));
}

void Router::put(std::string path, HttpHandler handler) {
    add_route(HttpMethod::Put, std::move(path), std::move(handler));
}

void Router::delete_route(std::string path, HttpHandler handler) {
    add_route(HttpMethod::Delete_, std::move(path), std::move(handler));
}

void Router::patch(std::string path, HttpHandler handler) {
    add_route(HttpMethod::Patch, std::move(path), std::move(handler));
}

HttpResponse Router::handle(const HttpRequest& request) const {
    for (const auto& route : routes_) {
        if (route.method == request.method &&
            route.path == request.target) {
            return route.handler(request);
        }
    }

    return HttpResponseBuilder{}
        .status(HttpStatus::NotFound)
        .body("Not Found")
        .build();
}

} // namespace edgex::http
