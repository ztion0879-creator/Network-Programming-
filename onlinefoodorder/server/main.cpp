
#include <iostream>
#include "Database.h"
#include "Auth.h"
#include "FoodService.h"
#include "OrderService.h"

int main() {
    Database database;

    if (!database.open("food_order.db")) {
        std::cout << "Cannot connect database!\n";
        return 1;
    }

    if (!database.initializeSchema()) {
        std::cout << "Cannot create tables!\n";
        return 1;
    }

    std::cout << "Database connected successfully!\n";

    Auth auth(database);
    FoodService foods(database);
    OrderService orders(database);

    int choice;

    do {
        std::cout << "\n===== ONLINE FOOD ORDERING =====\n";
        std::cout << "1. Register\n";
        std::cout << "2. Login\n";
        std::cout << "3. View foods\n";
        std::cout << "4. Add food (Admin demo)\n";
        std::cout << "5. Create order\n";
        std::cout << "6. View order history\n";
        std::cout << "7. Update order status (Admin demo)\n";
        std::cout << "0. Exit\n";
        std::cout << "Choose: ";

        std::cin >> choice;
        std::cin.ignore(10000, '\n');

        if (choice == 1) {
            std::string username, password, name, phone;

            std::cout << "Username: ";
            std::getline(std::cin, username);

            std::cout << "Password: ";
            std::getline(std::cin, password);

            std::cout << "Full name: ";
            std::getline(std::cin, name);

            std::cout << "Phone: ";
            std::getline(std::cin, phone);

            if (auth.registerUser(username, password, name, phone))
                std::cout << "Register successful!\n";
            else
                std::cout << "Register failed!\n";
        }

        else if (choice == 2) {
            std::string username, password, role;
            int userId = 0;

            std::cout << "Username: ";
            std::getline(std::cin, username);

            std::cout << "Password: ";
            std::getline(std::cin, password);

            if (auth.login(username, password, userId, role)) {
                std::cout << "Login successful!\n";
                std::cout << "User ID: " << userId << "\n";
                std::cout << "Role: " << role << "\n";
            }
            else {
                std::cout << "Login failed!\n";
            }
        }

        else if (choice == 3) {
            auto list = foods.getAllFoods();

            for (const auto& food : list) {
                std::cout << food.id << " | "
                    << food.name << " | "
                    << food.price << " | "
                    << (food.available ? "Available" : "Unavailable")
                    << "\n";
            }
        }

        else if (choice == 4) {
            std::string name, description, image;
            double price;

            std::cout << "Food name: ";
            std::getline(std::cin, name);

            std::cout << "Description: ";
            std::getline(std::cin, description);

            std::cout << "Price: ";
            std::cin >> price;
            std::cin.ignore(10000, '\n');

            std::cout << "Image path: ";
            std::getline(std::cin, image);

            if (foods.addFood(name, description, price, image))
                std::cout << "Added successfully!\n";
            else
                std::cout << "Add failed!\n";
        }

        else if (choice == 5) {
            int userId;
            std::string address;
            int count;

            std::cout << "User ID: ";
            std::cin >> userId;

            std::cout << "Number of food types: ";
            std::cin >> count;

            std::vector<CartItem> cart;

            for (int i = 0; i < count; i++) {
                CartItem item{};

                std::cout << "Food ID: ";
                std::cin >> item.foodId;

                std::cout << "Quantity: ";
                std::cin >> item.quantity;

                cart.push_back(item);
            }

            std::cin.ignore(10000, '\n');

            std::cout << "Delivery address: ";
            std::getline(std::cin, address);

            int orderId = 0;

            if (orders.createOrder(userId, cart, address, orderId))
                std::cout << "Order created! ID = " << orderId << "\n";
            else
                std::cout << "Create order failed!\n";
        }

        else if (choice == 6) {
            int userId;

            std::cout << "User ID: ";
            std::cin >> userId;

            auto list = orders.getOrders(userId);

            for (const auto& order : list) {
                std::cout << "Order ID: " << order.id
                    << " | Total: " << order.total
                    << " | Status: " << order.status
                    << " | Address: " << order.deliveryAddress
                    << "\n";
            }
        }

        else if (choice == 7) {
            int orderId;
            std::string status;

            std::cout << "Order ID: ";
            std::cin >> orderId;
            std::cin.ignore(10000, '\n');

            std::cout << "New status: ";
            std::getline(std::cin, status);

            if (orders.updateStatus(orderId, status))
                std::cout << "Status updated!\n";
            else
                std::cout << "Update failed!\n";
        }

    } while (choice != 0);

    return 0;
}
