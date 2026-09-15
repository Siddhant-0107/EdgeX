#include "edgex/http/http_response_builder.hpp"

#include <utility>

namespace edgex::http {

HttpResponseBuilder& HttpResponseBuilder::status(HttpStatus status) {
    response_.status = status;
    return *this;
}

HttpResponseBuilder& HttpResponseBuilder::header(std::string name,
                                                 std::string value) {
    response_.set_header(std::move(name), std::move(value));
    return *this;
}

HttpResponseBuilder& HttpResponseBuilder::body(std::string_view body) {
    response_.body.assign(body.begin(), body.end());
    return *this;
}

HttpResponseBuilder& HttpResponseBuilder::body(
    const std::vector<std::uint8_t>& body) {
    response_.body = body;
    return *this;
}

HttpResponse HttpResponseBuilder::build() const {
    return response_;
}

} // namespace edgex::http
