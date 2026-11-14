
#pragma once
#include "Database.h"
#include <string>

class Auth {
private:
    Database& database;

public:
    explicit Auth(Database& db);

    bool registerUser(
        const std::string& username,
        const std::string& password,
        const std::string& fullName,
        const std::string& phone
    );

    bool login(
        const std::string& username,
        const std::string& password,
        int& userId,
        std::string& role
    );
};
