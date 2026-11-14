
#include "Database.h"
#include <iostream>

Database::Database() : db(nullptr) {}

Database::~Database() {
    close();
}

bool Database::open(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Cannot open database: "
                  << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_busy_timeout(db, 5000);
    return true;
}

void Database::close() {
    if (db != nullptr) {
        sqlite3_close(db);
        db = nullptr;
    }
}

sqlite3* Database::getConnection() {
    return db;
}

bool Database::execute(const std::string& sql) {
    char* error = nullptr;

    int result = sqlite3_exec(
        db, sql.c_str(), nullptr, nullptr, &error
    );

    if (result != SQLITE_OK) {
        std::cerr << "SQL Error: "
                  << (error ? error : "Unknown")
                  << std::endl;

        sqlite3_free(error);
        return false;
    }

    return true;
}

bool Database::initializeSchema() {
    const char* sql = R"(
        PRAGMA foreign_keys = ON;

        CREATE TABLE IF NOT EXISTS Users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password TEXT NOT NULL,
            full_name TEXT NOT NULL,
            phone TEXT,
            address TEXT,
            role TEXT NOT NULL DEFAULT 'Customer'
                CHECK(role IN ('Customer','Admin'))
        );

        CREATE TABLE IF NOT EXISTS Foods (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            description TEXT,
            price REAL NOT NULL CHECK(price >= 0),
            image TEXT,
            available INTEGER NOT NULL DEFAULT 1
        );

        CREATE TABLE IF NOT EXISTS Orders (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            total REAL NOT NULL,
            delivery_address TEXT NOT NULL,
            status TEXT NOT NULL DEFAULT 'Pending'
                CHECK(status IN (
                    'Pending','Confirmed','Preparing',
                    'Delivering','Completed','Cancelled'
                )),
            created_at TEXT DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(user_id) REFERENCES Users(id)
        );

        CREATE TABLE IF NOT EXISTS OrderDetails (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            order_id INTEGER NOT NULL,
            food_id INTEGER NOT NULL,
            food_name TEXT NOT NULL,
            quantity INTEGER NOT NULL CHECK(quantity > 0),
            price REAL NOT NULL,
            FOREIGN KEY(order_id) REFERENCES Orders(id),
            FOREIGN KEY(food_id) REFERENCES Foods(id)
        );
    )";

    return execute(sql);
}
