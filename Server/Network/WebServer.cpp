#include "WebServer.h"
#include <iostream>
using namespace std;
WebServer::WebServer() {
    serverSocket = INVALID_SOCKET;
    winsockInitialized = false;
}
WebServer::~WebServer() {
    stop();
}
bool WebServer::start(int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "Loi khoi tao Winsock!" << endl;
        return false;
    }
    winsockInitialized = true;
    serverSocket = socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP
    );
    if (serverSocket == INVALID_SOCKET) {
        cout << "Khong the tao Socket!" << endl;
        stop();
        return false;
    }
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    if (bind(
        serverSocket,
        (sockaddr*)&serverAddress,
        sizeof(serverAddress)
    ) == SOCKET_ERROR) {
        cout << "Bind that bai! Loi: "
             << WSAGetLastError() << endl;
        stop();
        return false;
    }
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {

        cout << "Listen that bai!" << endl;
        stop();
        return false;
    }
    cout << "TCP Server da khoi dong!" << endl;
    cout << "Dang lang nghe tai port: " << port << endl;
    return true;
}
SOCKET WebServer::acceptClient() {
    if (serverSocket == INVALID_SOCKET) {
        return INVALID_SOCKET;
    }
    SOCKET clientSocket = accept(
        serverSocket,
        nullptr,
        nullptr
    );
    if (clientSocket == INVALID_SOCKET) {
        cout << "Accept that bai! Loi: "
             << WSAGetLastError() << endl;
    }
    else {
        cout << "Client da ket noi!" << endl;
    }
    return clientSocket;
}
void WebServer::stop() {
    if (serverSocket != INVALID_SOCKET) {
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
    }
    if (winsockInitialized) {
        WSACleanup();
        winsockInitialized = false;
    }
    cout << "TCP Server da dung." << endl;
}
