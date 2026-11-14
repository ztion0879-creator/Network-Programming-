#include "HttpResponse.h"
#include <sstream>
HttpResponse::HttpResponse() {
    version = "HTTP/1.1";
    statusCode = 200;
    statusMessage = "OK";
    body = "";
    headers["Connection"] = "close";
}
void HttpResponse::setStatus(int code, const string& message) {
    statusCode = code;
    statusMessage = message;
}
void HttpResponse::setHeader(const string& key, const string& value) {
    headers[key] = value;
}
void HttpResponse::setBody(const string& content) {
    body = content;
}
string HttpResponse::toString() {
    ostringstream response;

    response << version << " "
             << statusCode << " "
             << statusMessage << "\r\n";
    headers["Content-Length"] = to_string(body.size());
    for (const auto& header : headers) {
        response << header.first << ": "
                 << header.second << "\r\n";
    }
    response << "\r\n";
    response << body;
    return response.str();
}
