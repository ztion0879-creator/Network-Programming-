#include "Backend.h"
#include <algorithm>
#include <cstdlib>
using nlohmann::json;
Backend::Backend(const std::string& path):database(path),auth(database),foods(database),orders(database){}
bool Backend::initialize(){return database.isOpen()&&database.initialize();}
static std::string queryValue(const std::string& url,const std::string& key){
    auto q=url.find('?');if(q==std::string::npos)return "";
    std::string query=url.substr(q+1),needle=key+"=";auto p=query.find(needle);
    if(p==std::string::npos)return "";p+=needle.size();auto e=query.find('&',p);
    return query.substr(p,e==std::string::npos?std::string::npos:e-p);
}
json Backend::dispatch(const std::string& method,const std::string& rawUrl,const std::string& body,const std::string& token){
    std::string url=rawUrl.substr(0,rawUrl.find('?'));
    json data=json::object();
    try{if(!body.empty())data=json::parse(body);}catch(...){return {{"success",false},{"message","JSON body không hợp lệ."}};}
    if(method=="POST"&&url=="/api/register")return auth.registerUser(data);
    if(method=="POST"&&url=="/api/login")return auth.login(data);
    if(method=="POST"&&url=="/api/logout"){auth.logout(token);return {{"success",true},{"message","Đã đăng xuất."}};}
    if(method=="GET"&&url=="/api/foods")return foods.getFoods(queryValue(rawUrl,"search"));
    if(url=="/api/admin/foods"){
        if(!auth.isAdmin(token))return {{"success",false},{"http_status",403},{"message","Bạn không có quyền Admin."}};
        if(method=="POST")return foods.addFood(data);
        if(method=="PUT")return foods.updateFood(std::atoi(queryValue(rawUrl,"id").c_str()),data);
        if(method=="DELETE")return foods.deleteFood(std::atoi(queryValue(rawUrl,"id").c_str()));
    }
    if(url=="/api/orders"){
        json user=auth.userFromToken(token);
        if(user.is_null())return {{"success",false},{"http_status",401},{"message","Vui lòng đăng nhập."}};
        if(method=="POST")return orders.createOrder(user["id"].get<int>(),data);
        if(method=="GET")return orders.getOrders(user["id"].get<int>(),user["role"]=="Admin");
    }
    if(method=="PUT"&&url=="/api/orders/status"){
        if(!auth.isAdmin(token))return {{"success",false},{"http_status",403},{"message","Chỉ Admin được cập nhật trạng thái."}};
        return orders.updateStatus(data.value("order_id",0),data.value("status",""));
    }
    return {{"success",false},{"http_status",404},{"message","API không tồn tại."}};
}
