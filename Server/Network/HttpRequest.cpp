#include "HttpRequest.h"
#include <sstream>
HttpRequest::HttpRequest() {
    method = "";
    url = "";
    version = "";
    body = "";
}
bool HttpRequest::parse(const string& rawRequest) {
    istringstream requestStream(rawRequest);
    string requestLine;
    if (!getline(requestStream, requestLine)) {
        return false;
    }
    if (!requestLine.empty() && requestLine.back() == '\r') {
        requestLine.pop_back();
    }
    istringstream lineStream(requestLine);
    if (!(lineStream >> method >> url >> version)) {
        return false;
    }
    string headerLine;
    while (getline(requestStream, headerLine)) {
        if (!headerLine.empty() && headerLine.back() == '\r') {
            headerLine.pop_back();
        }
        if (headerLine.empty()) {
            break;
        }
        size_t separator = headerLine.find(':');
        if (separator != string::npos) {
            string key = headerLine.substr(0, separator);
            string value = headerLine.substr(separator + 1);
            size_t start = value.find_first_not_of(" \t");
            if (start != string::npos) {
                value = value.substr(start);
            } else {
                value = "";
            }
            headers[key] = value;
        }
    }
    string bodyLine;
    while (getline(requestStream, bodyLine)) {
        body += bodyLine;
        if (!requestStream.eof()) {
            body += "\n";
        }
    }
    return true;
}
