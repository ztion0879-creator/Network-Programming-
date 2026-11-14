
#pragma once
#include "Database.h"
#include <string>
#include <vector>

struct CartItem {
    int foodId;
    int quantity;
};

struct Order {
    int id;
    int userId;
    double total;
    std::string deliveryAddress;
    std::string status;
    std::string createdAt;
};

class OrderService {
private:
    Database& database;

public:
    explicit OrderService(Database& db);

    bool createOrder(
        int userId,
        const std::vector<CartItem>& cart,
        const std::string& deliveryAddress,
        int& newOrderId
    );

    std::vector<Order> getOrders(int userId);

    std::vector<Order> getAllOrders();

    bool updateStatus(
        int orderId,
        const std::string& status
    );
};
