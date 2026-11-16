#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <limits>
#include "Network/WebServer.h"
#include "Network/HttpRequest.h"
#include "Network/HttpResponse.h"
#include "Network/Router.h"
using namespace std;
const size_t MAX_HEADER_SIZE = 16 * 1024;       // 16 KB
const size_t MAX_BODY_SIZE = 1024 * 1024;        // 1 MB
string toLowerCase(string value) {
    transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char c) {
            return static_cast<char>(tolower(c));
        }
    );
    return value;
}
bool sendAll(SOCKET clientSocket, const string& data) {
    size_t totalSent = 0;
    while (totalSent < data.size()) {
        int sent = send(
            clientSocket,
            data.data() + totalSent,
            static_cast<int>(data.size() - totalSent),
            0
        );
        if (sent == SOCKET_ERROR || sent == 0) {
            return false;
        }
        totalSent += sent;
    }
    return true;
}
HttpResponse createErrorResponse(
    int statusCode,
    const string& statusMessage,
    const string& errorMessage
) {
    HttpResponse response;
    response.setStatus(statusCode, statusMessage);
    response.setHeader("Content-Type", "application/json");
    response.setBody(
        "{\"error\":\"" + errorMessage + "\"}"
    );
    return response;
}
bool getContentLength(
    const string& rawHeaders,
    size_t& contentLength
) {
    contentLength = 0;
    size_t lineStart = rawHeaders.find("\r\n");
    if (lineStart == string::npos) {
        return false;
    }
    lineStart += 2;
    bool found = false;
    while (lineStart < rawHeaders.size()) {
        size_t lineEnd = rawHeaders.find("\r\n", lineStart);

        if (lineEnd == string::npos) {
            lineEnd = rawHeaders.size();
        }
        string line = rawHeaders.substr(
            lineStart,
            lineEnd - lineStart
        );
        size_t separator = line.find(':');
        if (separator != string::npos) {
            string key = toLowerCase(
                line.substr(0, separator)
            );
            string value = line.substr(separator + 1);
            if (key == "content-length") {
                if (found) {
                    return false;
                }
                size_t start = value.find_first_not_of(" \t");
                if (start == string::npos) {
                    return false;
                }
                value = value.substr(start);
                size_t end = value.find_last_not_of(" \t");
                value = value.substr(0, end + 1);
                if (value.empty()) {
                    return false;
                }
                size_t result = 0;
                for (unsigned char c : value) {
                    if (!isdigit(c)) {
                        return false;
                    }
                    size_t digit = c - '0';
                   if (result >
                    ((std::numeric_limits<size_t>::max)() - digit) / 10) {
                    return false;
                 }
                    result = result * 10 + digit;
                }
                contentLength = result;
                found = true;
            }
        }
        lineStart = lineEnd + 2;
    }
    return true;
}
int main() {
    WebServer server;
    Router router;
    if (!server.start(8080)) {
        cout << "Khong the khoi dong Server!" << endl;
        return 1;
    }
    cout << "HTTP Server dang chay tai http://localhost:8080"
         << endl;
    while (true) {
        SOCKET clientSocket = server.acceptClient();

        if (clientSocket == INVALID_SOCKET) {
            continue;
        }
        string rawRequest;
        char buffer[4096];
        size_t headerEnd = string::npos;
        size_t contentLength = 0;
        bool requestError = false;
        int errorCode = 400;
        string errorMessage = "Bad Request";
        while (true) {
            int received = recv(
                clientSocket,
                buffer,
                sizeof(buffer),
                0
            );
            if (received == SOCKET_ERROR) {
                requestError = true;
                break;
            }
            if (received == 0) {
                break;
            }
            rawRequest.append(buffer, received);
            headerEnd = rawRequest.find("\r\n\r\n");
            if (headerEnd == string::npos) {
                if (rawRequest.size() > MAX_HEADER_SIZE) {
                    requestError = true;
                    errorCode = 431;
                    errorMessage = "Request Header Fields Too Large";
                    break;
                }
                continue;
            }
            if (headerEnd > MAX_HEADER_SIZE) {
                requestError = true;
                errorCode = 431;
                errorMessage = "Request Header Fields Too Large";
                break;
            }
            string rawHeaders = rawRequest.substr(
                0,
                headerEnd
            );
            if (!getContentLength(rawHeaders, contentLength)) {
                requestError = true;
                errorCode = 400;
                errorMessage = "Invalid Content-Length";
                break;
            }
            if (contentLength > MAX_BODY_SIZE) {
                requestError = true;
                errorCode = 413;
                errorMessage = "Payload Too Large";
                break;
            }
            size_t bodyStart = headerEnd + 4;
            if (rawRequest.size() >= bodyStart + contentLength) {
                break;
            }
        }
        if (!requestError) {
            if (headerEnd == string::npos) {
                requestError = true;
                errorMessage = "Incomplete HTTP Request";
            }
            else {
                size_t bodyStart = headerEnd + 4;
                if (rawRequest.size() < bodyStart + contentLength) {
                    requestError = true;
                    errorMessage = "Incomplete Request Body";
                }
            }
        }
        cout << "\n========== HTTP REQUEST =========="
             << endl;
        cout << rawRequest << endl;
        HttpResponse response;
        if (requestError) {
            response = createErrorResponse(
                errorCode,
                errorCode == 400 ? "Bad Request" :
                errorCode == 413 ? "Payload Too Large" :
                "Request Header Fields Too Large",
                errorMessage
            );
            cout << "HTTP Request khong hop le!" << endl;
        }
        else {
            HttpRequest request;
            if (request.parse(rawRequest)) {
                cout << "Method: " << request.method << endl;
                cout << "URL: " << request.url << endl;

                response = router.handleRequest(request);
            }
            else {
                response = createErrorResponse(
                    400,
                    "Bad Request",
                    "Cannot parse HTTP Request"
                );
            }
        }
        string responseData = response.toString();
        if (sendAll(clientSocket, responseData)) {
            cout << "Da gui HTTP Response!" << endl;
        }
        else {
            cout << "Gui HTTP Response that bai!" << endl;
        }
        closesocket(clientSocket);
    }
    return 0;
}
