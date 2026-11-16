#ifndef ROUTER_H
#define ROUTER_H
#include "HttpRequest.h"
#include "HttpResponse.h"
class Router {
public:
    HttpResponse handleRequest(const HttpRequest& request);
};
#endif
