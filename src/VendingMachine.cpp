#include "VendingMachine.h"

#include "Animation.h"
#include "Payment.h"

VendingMachine::VendingMachine() : products_(createProducts()) {}

const std::vector<Product>& VendingMachine::products() const {
    return products_;
}

int VendingMachine::selectedIndex() const {
    return selectedIndex_;
}

void VendingMachine::select(int index) {
    if (index >= 0 && index < static_cast<int>(products_.size())) {
        selectedIndex_ = index;
    }
}

const Product* VendingMachine::selectedProduct() const {
    if (products_.empty() || selectedIndex_ < 0 ||
        selectedIndex_ >= static_cast<int>(products_.size())) {
        return nullptr;
    }
    return &products_[static_cast<std::size_t>(selectedIndex_)];
}

glm::vec3 VendingMachine::productPosition(int index) const {
    const int column = index % 4;
    const int row = index / 4;
    return {-1.62f + static_cast<float>(column) * 0.82f,
            4.33f - static_cast<float>(row) * 1.32f,
            0.43f};
}

bool VendingMachine::vendSelected(Payment& payment, Animation& animation) {
    if (animation.active() || animation.pressing()) {
        status_ = "Máy đang nhả hàng. Vui lòng chờ.";
        return false;
    }
    if (animation.delivered()) {
        status_ = "Vui lòng nhận đồ uống trong khay trước.";
        return false;
    }
    Product* product = selectedIndex_ >= 0 &&
                               selectedIndex_ < static_cast<int>(products_.size())
                           ? &products_[static_cast<std::size_t>(selectedIndex_)]
                           : nullptr;
    if (!product) {
        status_ = "Vui lòng chọn sản phẩm.";
        return false;
    }
    if (product->stock <= 0) {
        status_ = "Sản phẩm này đã hết hàng.";
        return false;
    }
    if (payment.balance() + 0.001f < product->price) {
        status_ = "Vui lòng nạp thêm tiền để thanh toán.";
        return false;
    }
    if (!payment.pay(product->price)) {
        status_ = "Không thể hoàn tất thanh toán.";
        return false;
    }

    --product->stock;
    animation.start(selectedIndex_);
    status_ = "Đang đưa sản phẩm xuống khay nhận...";
    return true;
}

const char* VendingMachine::status() const {
    return status_;
}

void VendingMachine::setStatus(const char* message) {
    status_ = message;
}
