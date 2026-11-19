#ifndef ROUTER_H
#define ROUTER_H
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "../Backend/Backend.h"
class Router {
private:
    Backend& backend;
public:
    explicit Router(Backend& backend);

    HttpResponse handleRequest(const HttpRequest& request);
};
#endif