#include "Product.h"

std::vector<Product> createProducts() {
    return {
        {1, "Coca-Cola", ProductType::Can, 2.50f, 8, {0.78f, 0.035f, 0.045f}},
        {2, "Pepsi", ProductType::Can, 2.40f, 7, {0.045f, 0.16f, 0.62f}},
        {3, "Coca-Cola Zero", ProductType::Can, 2.50f, 6, {0.075f, 0.085f, 0.10f}},
        {4, "Pepsi Max", ProductType::Can, 2.40f, 5, {0.025f, 0.12f, 0.38f}},
        {5, "Nước khoáng", ProductType::Bottle, 1.50f, 9, {0.20f, 0.68f, 0.84f}},
        {6, "Nước suối", ProductType::Bottle, 1.75f, 8, {0.32f, 0.78f, 0.88f}},
        {7, "Nước cam", ProductType::Bottle, 2.25f, 6, {0.93f, 0.34f, 0.055f}},
        {8, "Snack khoai tây", ProductType::Snack, 1.75f, 5, {0.92f, 0.48f, 0.08f}},
        {9, "Bánh snack phô mai", ProductType::Snack, 2.00f, 4, {0.91f, 0.70f, 0.10f}},
        {10, "Bánh quy mặn", ProductType::Snack, 1.80f, 6, {0.62f, 0.27f, 0.09f}},
        {11, "Sô-cô-la", ProductType::Bakery, 2.25f, 7, {0.30f, 0.12f, 0.075f}},
        {12, "Bánh quy", ProductType::Bakery, 2.00f, 5, {0.76f, 0.48f, 0.21f}},
        {13, "Bánh muffin việt quất", ProductType::Bakery, 2.75f, 3, {0.34f, 0.26f, 0.52f}},
        {14, "Thanh ngũ cốc", ProductType::Bakery, 1.95f, 4, {0.52f, 0.34f, 0.15f}},
        {15, "Bánh sữa bơ", ProductType::Bakery, 2.50f, 3, {0.88f, 0.62f, 0.26f}},
    };
}
