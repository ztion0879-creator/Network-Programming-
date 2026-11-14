
#pragma once
#include "Database.h"
#include <string>
#include <vector>

struct Food {
    int id;
    std::string name;
    std::string description;
    double price;
    std::string image;
    bool available;
};

class FoodService {
private:
    Database& database;

public:
    explicit FoodService(Database& db);

    std::vector<Food> getAllFoods();

    bool addFood(
        const std::string& name,
        const std::string& description,
        double price,
        const std::string& image
    );

    bool updateFood(
        int id,
        const std::string& name,
        const std::string& description,
        double price,
        const std::string& image,
        bool available
    );

    bool deleteFood(int id);
};
