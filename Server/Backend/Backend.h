#pragma once
#include "Database.h"
#include "Auth.h"
#include "FoodService.h"
#include "OrderService.h"
#include <nlohmann/json.hpp>
#include <string>

// Lop facade de Thanh vien 1 goi tu Router: dispatch(method, url, body, token).
class Backend {
private:
    Database database;
    Auth auth;
    FoodService foods;
    OrderService orders;
public:
    explicit Backend(const std::string& databasePath = "database/food_order.db");
    bool initialize();
    nlohmann::json dispatch(const std::string& method,
                            const std::string& url,
                            const std::string& body,
                            const std::string& token);
};