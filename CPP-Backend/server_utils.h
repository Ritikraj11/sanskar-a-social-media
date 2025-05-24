#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// HttpRequest structure definition
struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
    std::unordered_map<std::string, std::string> queryParams;
};

// HttpResponse structure definition
struct HttpResponse {
    int status;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
};

#endif
