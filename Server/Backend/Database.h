#pragma once
#include <string>
#include <sqlite3.h>

class Database {
private:
    sqlite3* db;
    std::string error;
public:
    explicit Database(const std::string& path = "database/food_order.db");
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool isOpen() const;
    const std::string& lastError() const;
    sqlite3* connection() const;
    bool execute(const std::string& sql);
    bool initialize();
};
