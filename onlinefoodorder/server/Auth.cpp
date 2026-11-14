
#include "Auth.h"
#include <sqlite3.h>
#include <functional>
#include <sstream>
#include <iomanip>

Auth::Auth(Database& db) : database(db) {}

// Demo học tập: mã hóa mật khẩu bằng hash.
// Khi triển khai thực tế, nên thay bằng PBKDF2 hoặc Argon2.
static std::string hashPassword(const std::string& password) {
    std::size_t hashValue =
        std::hash<std::string>{}(password);

    std::ostringstream output;
    output << std::hex << hashValue;
    return output.str();
}

bool Auth::registerUser(
    const std::string& username,
    const std::string& password,
    const std::string& fullName,
    const std::string& phone
) {
    if (username.empty() || password.empty() ||
        fullName.empty()) {
        return false;
    }

    const char* sql =
        "INSERT INTO Users(username,password,full_name,phone) "
        "VALUES(?,?,?,?)";

    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = database.getConnection();

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr)
        != SQLITE_OK) {
        return false;
    }

    std::string hashed = hashPassword(password);

    sqlite3_bind_text(stmt, 1, username.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, hashed.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, fullName.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, phone.c_str(), -1,
                      SQLITE_TRANSIENT);

    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);

    return success;
}

bool Auth::login(
    const std::string& username,
    const std::string& password,
    int& userId,
    std::string& role
) {
    const char* sql =
        "SELECT id, role, password FROM Users "
        "WHERE username = ?";

    sqlite3_stmt* stmt = nullptr;
    sqlite3* db = database.getConnection();

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr)
        != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1,
                      SQLITE_TRANSIENT);

    bool success = false;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* saved =
            sqlite3_column_text(stmt, 2);

        if (saved &&
            hashPassword(password) ==
            reinterpret_cast<const char*>(saved)) {

            userId = sqlite3_column_int(stmt, 0);

            const unsigned char* roleText =
                sqlite3_column_text(stmt, 1);

            role = roleText
                ? reinterpret_cast<const char*>(roleText)
                : "Customer";

            success = true;
        }
    }

    sqlite3_finalize(stmt);
    return success;
}
