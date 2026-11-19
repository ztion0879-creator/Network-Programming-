#include "Router.h"
#include <algorithm>
#include <cctype>
#include <exception>
using nlohmann::json;
static std::string toLower(std::string value) {
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        }
    );
    return value;
}
static std::string getToken(const HttpRequest& request) {
    for (const auto& header : request.headers) {
        if (toLower(header.first) == "authorization") {
            const std::string prefix = "Bearer ";
            if (header.second.size() >= prefix.size() &&
                toLower(header.second.substr(0, prefix.size())) ==
                toLower(prefix)) {
                return header.second.substr(prefix.size());
            }
        }
    }
    return "";
}
Router::Router(Backend& backend) : backend(backend) {}
HttpResponse Router::handleRequest(const HttpRequest& request) {
    HttpResponse response;
    if (request.method == "OPTIONS") {
        response.setStatus(204, "No Content");
        response.setHeader("Content-Length", "0");
        response.setBody("");
        return response;
    }
    if (request.method == "GET" && request.url == "/") {
        response.setStatus(200, "OK");
        response.setHeader("Content-Type", "text/plain");
        response.setBody(
            "Online Food Ordering Server is running!"
        );
        return response;
    }
    try {
        std::string token = getToken(request);
        json result = backend.dispatch(
            request.method,
            request.url,
            request.body,
            token
        );
        int statusCode = result.value("http_status", 200);
        std::string statusMessage = "OK";
        if (statusCode == 400)
            statusMessage = "Bad Request";
        else if (statusCode == 401)
            statusMessage = "Unauthorized";
        else if (statusCode == 403)
            statusMessage = "Forbidden";
        else if (statusCode == 404)
            statusMessage = "Not Found";
        else if (statusCode >= 500)
            statusMessage = "Internal Server Error";
        response.setStatus(statusCode, statusMessage);
        response.setHeader("Content-Type", "application/json");
        response.setBody(result.dump());
    }
    catch (const std::exception& e) {
        json error = {
            {"success", false},
            {"message", e.what()}
        };
        response.setStatus(500, "Internal Server Error");
        response.setHeader("Content-Type", "application/json");
        response.setBody(error.dump());
    }
    return response;
}
