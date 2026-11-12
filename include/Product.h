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
