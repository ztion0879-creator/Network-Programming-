
#include "FoodService.h"
#include <sqlite3.h>

FoodService::FoodService(Database& db)
    : database(db) {
}

std::vector<Food> FoodService::getAllFoods() {
    std::vector<Food> foods;

    const char* sql =
        "SELECT id,name,description,price,image,available "
        "FROM Foods ORDER BY id";

    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = database.getConnection();

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr)
        != SQLITE_OK) {
        return foods;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Food food{};

        food.id = sqlite3_column_int(stmt, 0);

        const unsigned char* name =
            sqlite3_column_text(stmt, 1);
        const unsigned char* description =
            sqlite3_column_text(stmt, 2);
        const unsigned char* image =
            sqlite3_column_text(stmt, 4);

        food.name = name
            ? reinterpret_cast<const char*>(name) : "";

        food.description = description
            ? reinterpret_cast<const char*>(description) : "";

        food.price = sqlite3_column_double(stmt, 3);

        food.image = image
            ? reinterpret_cast<const char*>(image) : "";

        food.available = sqlite3_column_int(stmt, 5) != 0;

        foods.push_back(food);
    }

    sqlite3_finalize(stmt);
    return foods;
}

bool FoodService::addFood(
    const std::string& name,
    const std::string& description,
    double price,
    const std::string& image
) {
    if (name.empty() || price < 0) return false;

    const char* sql =
        "INSERT INTO Foods(name,description,price,image) "
        "VALUES(?,?,?,?)";

    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = database.getConnection();

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr)
        != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, description.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, price);
    sqlite3_bind_text(stmt, 4, image.c_str(), -1,
                      SQLITE_TRANSIENT);

    bool success = sqlite3_step(stmt) == SQLITE_DONE;

    sqlite3_finalize(stmt);
    return success;
}

bool FoodService::updateFood(
    int id,
    const std::string& name,
    const std::string& description,
    double price,
    const std::string& image,
    bool available
) {
    if (id <= 0 || name.empty() || price < 0)
        return false;

    const char* sql =
        "UPDATE Foods SET name=?,description=?,price=?,"
        "image=?,available=? WHERE id=?";

    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = database.getConnection();

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr)
        != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, description.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, price);
    sqlite3_bind_text(stmt, 4, image.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, available ? 1 : 0);
    sqlite3_bind_int(stmt, 6, id);

    bool success = sqlite3_step(stmt) == SQLITE_DONE
        && sqlite3_changes(db) > 0;

    sqlite3_finalize(stmt);
    return success;
}

bool FoodService::deleteFood(int id) {
    if (id <= 0) return false;

    const char* sql = "DELETE FROM Foods WHERE id=?";

    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = database.getConnection();

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr)
        != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);

    bool success = sqlite3_step(stmt) == SQLITE_DONE
        && sqlite3_changes(db) > 0;

    sqlite3_finalize(stmt);
    return success;
}
