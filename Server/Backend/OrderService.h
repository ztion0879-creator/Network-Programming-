#pragma once
#include "Database.h"
#include <nlohmann/json.hpp>
#include <string>

class OrderService {
private:
    Database& database;
public:
    explicit OrderService(Database& db);
    nlohmann::json createOrder(int userId, const nlohmann::json& data);
    nlohmann::json getOrders(int userId, bool admin = false);
    nlohmann::json updateStatus(int orderId, const std::string& status);
};
