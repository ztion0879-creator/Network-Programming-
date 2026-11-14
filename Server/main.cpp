#include <iostream>
#include "Network/WebServer.h"
using namespace std;
int main() {
    WebServer server;
    if (!server.start(8080)) {
        cout << "Khong the khoi dong Server!" << endl;
        return 1;
    }
    cout << "Server dang cho Client ket noi..." << endl;
    while (true) {
        SOCKET clientSocket = server.acceptClient();
        if (clientSocket == INVALID_SOCKET) {
            continue;
        }
        cout << "Da nhan ket noi tu Client!" << endl;
        closesocket(clientSocket);
        cout << "Da dong ket noi Client." << endl;
    }
    return 0;
}
