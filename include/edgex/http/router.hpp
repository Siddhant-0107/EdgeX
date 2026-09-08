#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "edgex/http/http_request.hpp"
#include "edgex/http/http_response.hpp"

namespace edgex::http {

using HttpHandler = std::function<HttpResponse(const HttpRequest&)>;

class Router {
public:
    void add_route(HttpMethod method,
                   std::string path,
                   HttpHandler handler);

    void get(std::string path, HttpHandler handler);

    void post(std::string path, HttpHandler handler);

    void put(std::string path, HttpHandler handler);

    void delete_route(std::string path, HttpHandler handler);

    void patch(std::string path, HttpHandler handler);

    [[nodiscard]]
    HttpResponse handle(const HttpRequest& request) const;

private:
    struct Route {
        HttpMethod method;
        std::string path;
        HttpHandler handler;
    };

    std::vector<Route> routes_;
};

} // namespace edgex::http
