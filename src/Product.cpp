#include "Product.h"

std::vector<Product> createProducts() {
    return {
        {1, "Coca-Cola", ProductType::Can, 2.50f, 8, {0.82f, 0.07f, 0.10f}},
        {2, "Pepsi", ProductType::Can, 2.40f, 7, {0.08f, 0.22f, 0.66f}},
        {3, "Energy Pop", ProductType::Can, 2.75f, 6, {0.70f, 0.78f, 0.24f}},
        {4, "Lemon Soda", ProductType::Can, 2.20f, 7, {0.90f, 0.72f, 0.20f}},
        {5, "Nước khoáng", ProductType::Bottle, 1.50f, 9, {0.32f, 0.76f, 0.88f}},
        {6, "Nước cam", ProductType::Bottle, 2.25f, 6, {0.96f, 0.43f, 0.15f}},
        {7, "Nước táo", ProductType::Bottle, 2.10f, 6, {0.64f, 0.78f, 0.28f}},
        {8, "Trà đào", ProductType::Bottle, 2.35f, 5, {0.90f, 0.53f, 0.45f}},
        {9, "Trà sữa", ProductType::Cup, 3.00f, 5, {0.74f, 0.52f, 0.35f}},
        {10, "Sữa dâu", ProductType::Bottle, 2.60f, 4, {0.91f, 0.52f, 0.66f}},
        {11, "Cà phê lạnh", ProductType::Can, 2.80f, 5, {0.42f, 0.24f, 0.16f}},
        {12, "Matcha latte", ProductType::Cup, 3.10f, 4, {0.45f, 0.68f, 0.38f}},
    };
}
