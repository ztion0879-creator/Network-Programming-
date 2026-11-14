
#pragma once
#include <string>
#include <sqlite3.h>

class Database {
private:
    sqlite3* db;

public:
    Database();
    ~Database();

    bool open(const std::string& path);
    void close();
    bool initializeSchema();

    sqlite3* getConnection();
    bool execute(const std::string& sql);
};
