#include "edgex/static/static_file_server.hpp"

#include "edgex/http/http_response_builder.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace edgex::static_files {

namespace {

bool is_path_inside(const std::filesystem::path& path,
                    const std::filesystem::path& root) {
    auto path_it = path.begin();
    auto root_it = root.begin();

    for (; root_it != root.end(); ++root_it, ++path_it) {
        if (path_it == path.end() || *path_it != *root_it) {
            return false;
        }
    }

    return true;
}

std::string mime_type_for(const std::filesystem::path& path) {
    std::string extension = path.extension().string();

    std::transform(
        extension.begin(),
        extension.end(),
        extension.begin(),
        [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });

    if (extension == ".html" || extension == ".htm") {
        return "text/html";
    }

    if (extension == ".css") {
        return "text/css";
    }

    if (extension == ".js") {
        return "application/javascript";
    }

    if (extension == ".json") {
        return "application/json";
    }

    if (extension == ".txt") {
        return "text/plain";
    }

    if (extension == ".xml") {
        return "application/xml";
    }

    if (extension == ".jpg" || extension == ".jpeg") {
        return "image/jpeg";
    }

    if (extension == ".png") {
        return "image/png";
    }

    if (extension == ".gif") {
        return "image/gif";
    }

    if (extension == ".svg") {
        return "image/svg+xml";
    }

    if (extension == ".ico") {
        return "image/x-icon";
    }

    if (extension == ".pdf") {
        return "application/pdf";
    }

    return "application/octet-stream";
}

std::string strip_query_string(std::string_view target) {
    const auto query_position = target.find('?');

    if (query_position == std::string_view::npos) {
        return std::string(target);
    }

    return std::string(target.substr(0, query_position));
}

http::HttpResponse error_response(http::HttpStatus status,
                                  std::string_view message) {
    return http::HttpResponseBuilder{}
        .status(status)
        .body(message)
        .build();
}

} // namespace

StaticFileServer::StaticFileServer(
    std::filesystem::path document_root)
    : document_root_(std::move(document_root)) {}

http::HttpResponse StaticFileServer::serve(
    const http::HttpRequest& request) const {

    if (request.method != http::HttpMethod::Get) {
        return error_response(
            http::HttpStatus::MethodNotAllowed,
            "Method Not Allowed");
    }

    const std::string request_path =
        strip_query_string(request.target);

    if (request_path.empty() || request_path.front() != '/') {
        return error_response(
            http::HttpStatus::BadRequest,
            "Bad Request");
    }

    const std::filesystem::path relative_path(
        request_path.substr(1));

    std::error_code error;

    const auto canonical_root =
        std::filesystem::weakly_canonical(
            document_root_,
            error);

    if (error) {
        return error_response(
            http::HttpStatus::InternalServerError,
            "Internal Server Error");
    }

    const auto requested_path =
        std::filesystem::weakly_canonical(
            canonical_root / relative_path,
            error);

    if (error) {
        return error_response(
            http::HttpStatus::NotFound,
            "Not Found");
    }

    if (!is_path_inside(requested_path, canonical_root)) {
        return error_response(
            http::HttpStatus::NotFound,
            "Not Found");
    }

    if (!std::filesystem::is_regular_file(
            requested_path, error)) {
        return error_response(
            http::HttpStatus::NotFound,
            "Not Found");
    }

    std::ifstream file(
        requested_path,
        std::ios::binary);

    if (!file) {
        return error_response(
            http::HttpStatus::InternalServerError,
            "Internal Server Error");
    }

    file.seekg(0, std::ios::end);

    const auto file_size = file.tellg();

    if (file_size < 0) {
        return error_response(
            http::HttpStatus::InternalServerError,
            "Internal Server Error");
    }

    file.seekg(0, std::ios::beg);

    std::vector<std::uint8_t> contents(
        static_cast<std::size_t>(file_size));

    if (!contents.empty()) {
        file.read(
            reinterpret_cast<char*>(contents.data()),
            static_cast<std::streamsize>(contents.size()));

        if (!file) {
            return error_response(
                http::HttpStatus::InternalServerError,
                "Internal Server Error");
        }
    }

    return http::HttpResponseBuilder{}
        .status(http::HttpStatus::Ok)
        .header("Content-Type", mime_type_for(requested_path))
        .body(contents)
        .build();
}

} // namespace edgex::static_files
