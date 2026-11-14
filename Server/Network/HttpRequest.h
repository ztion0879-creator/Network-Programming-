#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H
#include <string>
#include <map>
using namespace std;
class HttpRequest {
public:
    string method;
    string url;
    string version;
    map<string, string> headers;
    string body;
    HttpRequest();
    bool parse(const string& rawRequest);
};
#endif
