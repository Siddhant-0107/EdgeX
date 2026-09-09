#pragma once

#include <filesystem>

#include "edgex/http/http_request.hpp"
#include "edgex/http/http_response.hpp"

namespace edgex::static_files {

class StaticFileServer {
public:
    explicit StaticFileServer(std::filesystem::path document_root);

    [[nodiscard]]
    http::HttpResponse serve(const http::HttpRequest& request) const;

private:
    std::filesystem::path document_root_;
};

} // namespace edgex::static_files
