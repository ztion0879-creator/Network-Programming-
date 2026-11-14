#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H
#include <string>
#include <map>
using namespace std;
class HttpResponse {
public:
    string version;
    int statusCode;
    string statusMessage;
    map<string, string> headers;
    string body;
    HttpResponse();
    void setStatus(int code, const string& message);
    void setHeader(const string& key, const string& value);
    void setBody(const string& content);
    string toString();
};
#endif
