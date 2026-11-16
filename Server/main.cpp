
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include "Network/WebServer.h"
#include "Network/HttpRequest.h"
#include "Network/HttpResponse.h"
#include "Network/Router.h"
using namespace std;
int main() {
    WebServer server;
    Router router;
    if (!server.start(8080)) {
        cout << "Khong the khoi dong Server!" << endl;
        return 1;
    }
    cout << "HTTP Server dang chay tai http://localhost:8080" << endl;
    while (true) {
        SOCKET clientSocket = server.acceptClient();
        if (clientSocket == INVALID_SOCKET) {
            continue;
        }
        string rawRequest;
        char buffer[4096];
        int received;
        size_t headerEnd = string::npos;
        size_t contentLength = 0;
        bool hasContentLength = false;
        while ((received = recv(
            clientSocket,
            buffer,
            sizeof(buffer),
            0
        )) > 0) {
            rawRequest.append(buffer, received);
            headerEnd = rawRequest.find("\r\n\r\n");
            if (headerEnd != string::npos) {
                string headersPart = rawRequest.substr(0, headerEnd);
                string lowerHeaders = headersPart;
                transform(
                    lowerHeaders.begin(),
                    lowerHeaders.end(),
                    lowerHeaders.begin(),
                    [](unsigned char c) {
                        return static_cast<char>(tolower(c));
                    }
                );
                size_t pos = lowerHeaders.find("content-length:");
                if (pos != string::npos) {
                    size_t valueStart = pos + 15;
                    size_t lineEnd = lowerHeaders.find("\r\n", valueStart);
                    string value = lowerHeaders.substr(
                        valueStart,
                        lineEnd == string::npos
                            ? string::npos
                            : lineEnd - valueStart
                    );
                    contentLength = stoull(value);
                    hasContentLength = true;
                }
                size_t bodyStart = headerEnd + 4;
                if (hasContentLength) {
                    if (rawRequest.size() >= bodyStart + contentLength) {
                        break;
                    }
                }
                else {
                    break;
                }
            }
        }
        cout << "\n========== HTTP REQUEST ==========" << endl;
        cout << rawRequest << endl;
        HttpRequest request;
        if (headerEnd != string::npos && request.parse(rawRequest)) {
            cout << "Method: " << request.method << endl;
            cout << "URL: " << request.url << endl;
            HttpResponse response = router.handleRequest(request);
            string responseData = response.toString();
            int totalSent = 0;
            while (totalSent < (int)responseData.size()) {
                int sent = send(
                    clientSocket,
                    responseData.c_str() + totalSent,
                    (int)responseData.size() - totalSent,
                    0
                );
                if (sent == SOCKET_ERROR || sent == 0) {
                    break;
                }
                totalSent += sent;
            }
            cout << "Da gui HTTP Response!" << endl;
        }
        else {
            cout << "HTTP Request khong hop le!" << endl;
        }
        closesocket(clientSocket);
    }
    return 0;
}
