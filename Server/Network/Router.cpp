#include "Router.h"
HttpResponse Router::handleRequest(const HttpRequest& request) {
    HttpResponse response;
    if (request.method == "GET" && request.url == "/") {
        response.setStatus(200, "OK");
        response.setHeader("Content-Type", "text/plain");
        response.setBody("Online Food Ordering Server is running!");
    }
    else if (request.method == "GET" && request.url == "/api/foods") {
        response.setStatus(200, "OK");
        response.setHeader("Content-Type", "application/json");
        response.setBody(
            R"([
                {"id":1,"name":"Pizza","price":100000},
                {"id":2,"name":"Burger","price":50000},
                {"id":3,"name":"Chicken","price":80000}
            ])"
        );
    }
    else if (request.method == "POST" && request.url == "/api/login") {
        response.setStatus(200, "OK");
        response.setHeader("Content-Type", "application/json");
        response.setBody(
            R"({"message":"Login API is working"})"
        );
    }
    else {
        response.setStatus(404, "Not Found");
        response.setHeader("Content-Type", "application/json");
        response.setBody(
            R"({"error":"API not found"})"
        );
    }
    return response;
}
