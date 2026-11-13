#pragma once

#include <glm/vec3.hpp>
#include <string>
#include <vector>

enum class ProductType {
    Can,
    Bottle,
    Cup,
    Snack,
    Bakery,
};

struct Product {
    int id{};
    std::string name;
    ProductType type{ProductType::Can};
    float price{};
    int stock{};
    glm::vec3 color{1.0f};
};

std::vector<Product> createProducts();

inline std::string productCode(int index) {
    return std::string(1, static_cast<char>('A' + index / 4)) + std::to_string(index % 4 + 1);
}
