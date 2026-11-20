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
    int hoveredIndex() const { return hoveredIndex_; }
    void hover(int index) { hoveredIndex_ = index; }
    void select(int index);
    const Product* selectedProduct() const;
    glm::vec3 productPosition(int index) const;
    bool vendSelected(Payment& payment, Animation& animation);
    bool addStock(int index);
    void setServiceDoorOpen(bool open) { serviceDoorOpen_ = open; }
    bool serviceDoorOpen() const { return serviceDoorOpen_; }
    const char* status() const;
    void setStatus(const char* message);

private:
    std::vector<Product> products_;
    int selectedIndex_{};
    int hoveredIndex_{-1};
    const char* status_{"Vui lòng chọn sản phẩm."};
    bool serviceDoorOpen_{};
};
