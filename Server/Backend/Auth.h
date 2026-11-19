#pragma once
#include "Database.h"
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>

class Auth {
private:
    Database& database;
    std::unordered_map<std::string, int> sessions; // token -> user id
    std::string hashPassword(const std::string& password) const;
public:
    explicit Auth(Database& db);
    nlohmann::json registerUser(const nlohmann::json& data);
    nlohmann::json login(const nlohmann::json& data);
    nlohmann::json userFromToken(const std::string& token);
    bool isAdmin(const std::string& token);
    void logout(const std::string& token);
};