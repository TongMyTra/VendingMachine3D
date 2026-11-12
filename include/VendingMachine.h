#pragma once

#include "Product.h"
#include <glm/vec3.hpp>
#include <vector>

class Animation;
class Payment;

class VendingMachine {
public:
    VendingMachine();

    const std::vector<Product>& products() const;
    int selectedIndex() const;
    void select(int index);
    const Product* selectedProduct() const;
    glm::vec3 productPosition(int index) const;
    bool vendSelected(Payment& payment, Animation& animation);
    const char* status() const;
    void setStatus(const char* message);

private:
    std::vector<Product> products_;
    int selectedIndex_{};
    const char* status_{"Vui lòng chọn sản phẩm."};
};
