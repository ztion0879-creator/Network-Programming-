#ifndef WEBSERVER_H
#define WEBSERVER_H
#include <winsock2.h>
class WebServer {
private:
    SOCKET serverSocket;
    bool winsockInitialized;
public:
    WebServer();
    ~WebServer();
    bool start(int port);
    SOCKET acceptClient();
    void stop();
};
#endif
