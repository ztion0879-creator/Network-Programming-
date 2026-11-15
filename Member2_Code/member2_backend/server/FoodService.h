#pragma once
#include "Database.h"
#include <nlohmann/json.hpp>
#include <string>

class FoodService {
private:
    Database& database;
public:
    explicit FoodService(Database& db);
    nlohmann::json getFoods(const std::string& search = "");
    nlohmann::json addFood(const nlohmann::json& data);
    nlohmann::json updateFood(int id, const nlohmann::json& data);
    nlohmann::json deleteFood(int id);
};
