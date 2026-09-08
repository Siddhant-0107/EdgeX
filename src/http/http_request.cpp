#include "edgex/http/http_request.hpp"

namespace edgex::http {
namespace {

char to_lower_ascii(char ch) {
    if (ch >= 'A' && ch <= 'Z') {
        return static_cast<char>(ch - 'A' + 'a');
    }

    return ch;
}

bool equals_case_insensitive_ascii(std::string_view lhs, std::string_view rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (std::size_t i = 0; i < lhs.size(); ++i) {
        if (to_lower_ascii(lhs[i]) != to_lower_ascii(rhs[i])) {
            return false;
        }
    }

    return true;
}

std::string_view trim_ows(std::string_view input) {
    std::size_t begin = 0;
    while (begin < input.size() && (input[begin] == ' ' || input[begin] == '\t')) {
        ++begin;
    }

    std::size_t end = input.size();
    while (end > begin && (input[end - 1] == ' ' || input[end - 1] == '\t')) {
        --end;
    }

    return input.substr(begin, end - begin);
}

}  // namespace

bool HttpRequest::has_header(std::string_view name) const {
    for (const HeaderField& header : headers) {
        if (equals_case_insensitive_ascii(header.name, name)) {
            return true;
        }
    }

    return false;
}

std::optional<std::string_view> HttpRequest::header_value(std::string_view name) const {
    for (const HeaderField& header : headers) {
        if (equals_case_insensitive_ascii(header.name, name)) {
            return trim_ows(std::string_view(header.value));
        }
    }

    return std::nullopt;
}

}  // namespace edgex::http
