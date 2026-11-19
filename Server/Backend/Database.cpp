#include "Database.h"
#include <filesystem>

Database::Database(const std::string& path) : db(nullptr) {
    std::filesystem::path p(path);
    if (p.has_parent_path()) std::filesystem::create_directories(p.parent_path());
    if (sqlite3_open(path.c_str(), &db) != SQLITE_OK) {
        error = db ? sqlite3_errmsg(db) : "SQLite open failed";
        if (db) { sqlite3_close(db); db = nullptr; }
    } else {
        sqlite3_busy_timeout(db, 5000);
        execute("PRAGMA foreign_keys = ON;");
    }
}
Database::~Database() { if (db) sqlite3_close(db); }
bool Database::isOpen() const { return db != nullptr; }
const std::string& Database::lastError() const { return error; }
sqlite3* Database::connection() const { return db; }
bool Database::execute(const std::string& sql) {
    if (!db) { error = "Database is not open"; return false; }
    char* message = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &message);
    if (rc != SQLITE_OK) {
        error = message ? message : sqlite3_errmsg(db);
        sqlite3_free(message);
        return false;
    }
     error.clear();
    return true;
}

bool Database::initialize() {
    if (!execute(R"SQL(
        PRAGMA foreign_keys = ON;
        CREATE TABLE IF NOT EXISTS Users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL UNIQUE,
            password_hash TEXT NOT NULL,
            full_name TEXT NOT NULL,
            phone TEXT DEFAULT '',
            address TEXT DEFAULT '',
            role TEXT NOT NULL DEFAULT 'Customer'
                CHECK(role IN ('Customer','Admin')),
            created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
        );
        CREATE TABLE IF NOT EXISTS Foods (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            description TEXT DEFAULT '',
            price REAL NOT NULL CHECK(price >= 0),
            image TEXT DEFAULT '',
            category TEXT DEFAULT '',
            available INTEGER NOT NULL DEFAULT 1 CHECK(available IN (0,1)),
            created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
        );
        CREATE TABLE IF NOT EXISTS Orders (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            total REAL NOT NULL CHECK(total >= 0),
            delivery_address TEXT NOT NULL,
            note TEXT DEFAULT '',
            status TEXT NOT NULL DEFAULT 'Pending'
                CHECK(status IN ('Pending','Confirmed','Preparing','Delivering','Completed','Cancelled')),
            created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(user_id) REFERENCES Users(id)
        );
        CREATE TABLE IF NOT EXISTS OrderDetails (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            order_id INTEGER NOT NULL,
            food_id INTEGER NOT NULL,
            food_name TEXT NOT NULL,
            unit_price REAL NOT NULL,
            quantity INTEGER NOT NULL CHECK(quantity > 0),
            subtotal REAL NOT NULL,
            FOREIGN KEY(order_id) REFERENCES Orders(id) ON DELETE CASCADE,
            FOREIGN KEY(food_id) REFERENCES Foods(id)
        );
        CREATE INDEX IF NOT EXISTS idx_orders_user ON Orders(user_id);
        CREATE INDEX IF NOT EXISTS idx_order_details_order ON OrderDetails(order_id);
    )SQL")) {
        return false;
    }
    return execute(R"SQL(
        INSERT INTO Foods
            (name, description, price, image, category, available)
        SELECT 'Pizza', 'Pizza phô mai', 100000, '', 'Fast Food', 1
        WHERE NOT EXISTS (SELECT 1 FROM Foods)

        UNION ALL

        SELECT 'Burger', 'Burger bò phô mai', 50000, '', 'Fast Food', 1
        WHERE NOT EXISTS (SELECT 1 FROM Foods)

        UNION ALL

        SELECT 'Chicken', 'Gà rán giòn', 80000, '', 'Fast Food', 1
        WHERE NOT EXISTS (SELECT 1 FROM Foods);
    )SQL");
}
