
#include "OrderService.h"
#include <sqlite3.h>

OrderService::OrderService(Database& db)
    : database(db) {
}

bool OrderService::createOrder(
    int userId,
    const std::vector<CartItem>& cart,
    const std::string& deliveryAddress,
    int& newOrderId
) {
    if (userId <= 0 || cart.empty() ||
        deliveryAddress.empty()) {
        return false;
    }

    sqlite3* db = database.getConnection();

    if (!database.execute("BEGIN TRANSACTION"))
        return false;

    double total = 0;

    // Kiểm tra món ăn và tính tổng tiền từ giá trong database.
    const char* foodSql =
        "SELECT price, name FROM Foods "
        "WHERE id=? AND available=1";

    sqlite3_stmt* foodStmt = nullptr;

    if (sqlite3_prepare_v2(db, foodSql, -1,
                           &foodStmt, nullptr) != SQLITE_OK) {
        database.execute("ROLLBACK");
        return false;
    }

    for (const auto& item : cart) {
        if (item.foodId <= 0 || item.quantity <= 0) {
            sqlite3_finalize(foodStmt);
            database.execute("ROLLBACK");
            return false;
        }

        sqlite3_reset(foodStmt);
        sqlite3_clear_bindings(foodStmt);
        sqlite3_bind_int(foodStmt, 1, item.foodId);

        if (sqlite3_step(foodStmt) != SQLITE_ROW) {
            sqlite3_finalize(foodStmt);
            database.execute("ROLLBACK");
            return false;
        }

        total += sqlite3_column_double(foodStmt, 0)
                 * item.quantity;
    }

    sqlite3_finalize(foodStmt);

    // Tạo đơn hàng.
    const char* orderSql =
        "INSERT INTO Orders(user_id,total,delivery_address,status) "
        "VALUES(?,?,?,'Pending')";

    sqlite3_stmt* orderStmt = nullptr;

    if (sqlite3_prepare_v2(db, orderSql, -1,
                           &orderStmt, nullptr) != SQLITE_OK) {
        database.execute("ROLLBACK");
        return false;
    }

    sqlite3_bind_int(orderStmt, 1, userId);
    sqlite3_bind_double(orderStmt, 2, total);
    sqlite3_bind_text(orderStmt, 3,
        deliveryAddress.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(orderStmt) != SQLITE_DONE) {
        sqlite3_finalize(orderStmt);
        database.execute("ROLLBACK");
        return false;
    }

    sqlite3_finalize(orderStmt);

    newOrderId = static_cast<int>(
        sqlite3_last_insert_rowid(db)
    );

    // Lưu chi tiết đơn hàng và giá tại thời điểm đặt.
    const char* detailSql =
        "INSERT INTO OrderDetails("
        "order_id,food_id,food_name,quantity,price) "
        "SELECT ?,id,name,?,price FROM Foods "
        "WHERE id=? AND available=1";

    sqlite3_stmt* detailStmt = nullptr;

    if (sqlite3_prepare_v2(db, detailSql, -1,
                           &detailStmt, nullptr) != SQLITE_OK) {
        database.execute("ROLLBACK");
        return false;
    }

    for (const auto& item : cart) {
        sqlite3_reset(detailStmt);
        sqlite3_clear_bindings(detailStmt);

        sqlite3_bind_int(detailStmt, 1, newOrderId);
        sqlite3_bind_int(detailStmt, 2, item.quantity);
        sqlite3_bind_int(detailStmt, 3, item.foodId);

        if (sqlite3_step(detailStmt) != SQLITE_DONE ||
            sqlite3_changes(db) != 1) {
            sqlite3_finalize(detailStmt);
            database.execute("ROLLBACK");
            return false;
        }
    }

    sqlite3_finalize(detailStmt);

    if (!database.execute("COMMIT")) {
        database.execute("ROLLBACK");
        return false;
    }

    return true;
}

std::vector<Order> OrderService::getOrders(int userId) {
    std::vector<Order> orders;

    const char* sql =
        "SELECT id,user_id,total,delivery_address,"
        "status,created_at FROM Orders "
        "WHERE user_id=? ORDER BY id DESC";

    sqlite3* db = database.getConnection();
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr)
        != SQLITE_OK) {
        return orders;
    }

    sqlite3_bind_int(stmt, 1, userId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Order order{};

        order.id = sqlite3_column_int(stmt, 0);
        order.userId = sqlite3_column_int(stmt, 1);
        order.total = sqlite3_column_double(stmt, 2);

        const unsigned char* address =
            sqlite3_column_text(stmt, 3);
        const unsigned char* status =
            sqlite3_column_text(stmt, 4);
        const unsigned char* created =
            sqlite3_column_text(stmt, 5);

        order.deliveryAddress = address
            ? reinterpret_cast<const char*>(address) : "";

        order.status = status
            ? reinterpret_cast<const char*>(status) : "";

        order.createdAt = created
            ? reinterpret_cast<const char*>(created) : "";

        orders.push_back(order);
    }

    sqlite3_finalize(stmt);
    return orders;
}

std::vector<Order> OrderService::getAllOrders() {
    std::vector<Order> orders;

    const char* sql =
        "SELECT id,user_id,total,delivery_address,"
        "status,created_at FROM Orders ORDER BY id DESC";

    sqlite3* db = database.getConnection();
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr)
        != SQLITE_OK) {
        return orders;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Order order{};

        order.id = sqlite3_column_int(stmt, 0);
        order.userId = sqlite3_column_int(stmt, 1);
        order.total = sqlite3_column_double(stmt, 2);

        const unsigned char* address =
            sqlite3_column_text(stmt, 3);
        const unsigned char* status =
            sqlite3_column_text(stmt, 4);
        const unsigned char* created =
            sqlite3_column_text(stmt, 5);

        order.deliveryAddress = address
            ? reinterpret_cast<const char*>(address) : "";

        order.status = status
            ? reinterpret_cast<const char*>(status) : "";

        order.createdAt = created
            ? reinterpret_cast<const char*>(created) : "";

        orders.push_back(order);
    }

    sqlite3_finalize(stmt);
    return orders;
}

bool OrderService::updateStatus(
    int orderId,
    const std::string& status
) {
    const std::vector<std::string> allowed = {
        "Pending",
        "Confirmed",
        "Preparing",
        "Delivering",
        "Completed",
        "Cancelled"
    };

    bool valid = false;

    for (const auto& value : allowed) {
        if (value == status) {
            valid = true;
            break;
        }
    }

    if (!valid || orderId <= 0)
        return false;

    const char* sql =
        "UPDATE Orders SET status=? WHERE id=?";

    sqlite3* db = database.getConnection();
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr)
        != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, status.c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, orderId);

    bool success = sqlite3_step(stmt) == SQLITE_DONE
        && sqlite3_changes(db) > 0;

    sqlite3_finalize(stmt);
    return success;
}
