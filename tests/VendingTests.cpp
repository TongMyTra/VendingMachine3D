#include "Animation.h"
#include "CashInteraction.h"
#include "Payment.h"
#include "VendingMachine.h"
#include "Interaction.h"
#include "Camera.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    VendingMachine machine;
    Animation animation;
    Payment payment;
    Interaction interaction(machine, payment, animation);
    assert(productCode(0) == "A1" && productCode(7) == "B4" && productCode(11) == "C4");
    assert(!interaction.purchase()); // Insufficient money: no stock or motion changes.
    assert(!animation.active() && machine.products()[0].stock == productSlotCapacity);
    interaction.select(5);
    assert(animation.pressing());
    assert(!interaction.purchase());
    animation.update(0.45f);
    assert(animation.buttonDepth(5) > 0.025f);
    assert(animation.buttonDepth(4) == 0);
    animation.update(0.6f);
    assert(!animation.pressing());
    assert(animation.buttonDepth(5) == 0);
    payment.insert(50000);
    const float expectedChange = 50000 - machine.products()[5].price;
    assert(interaction.purchase());
    assert(animation.active() && machine.products()[5].stock == 5);
    assert(payment.balance() == 0 && payment.change() == expectedChange);
    assert(!interaction.purchase()); // No concurrent vending.
    const auto shelf = machine.productPosition(5);
    assert(glm::length(animation.deliveryPosition(shelf) - shelf) < 0.001f);
    animation.update(0.4f);
    const auto pushed = animation.deliveryPosition(shelf);
    assert(pushed.z > shelf.z && std::abs(pushed.y - shelf.y) < 0.001f);
    animation.update(0.5f);
    const auto falling = animation.deliveryPosition(shelf);
    assert(falling.y < shelf.y && animation.deliveryRotation() > 0);
    for (int i = 0; i < 200; ++i) {
        animation.update(0.01f);
        const auto p = animation.deliveryPosition(shelf);
        assert(std::isfinite(p.x) && p.y >= 0.48f);
    }
    assert(!animation.active() && animation.delivered());
    assert(animation.takeArrival() && !animation.takeArrival());
    assert(glm::length(animation.deliveryPosition(shelf) - glm::vec3(-0.43f, 0.49f, 1.17f)) < 0.001f);
    assert(std::abs(animation.deliveryRotation() - 90.0f) < 0.001f);
    assert(!interaction.purchase()); // Tray must be collected first.
    CashInteraction cash;
    assert(cash.returnChange(payment));
    assert(!cash.returnChange(payment));
    cash.update(1.0f, payment);
    assert(payment.change() == expectedChange);
    assert(cash.state() == CashInteraction::State::AwaitingCollection);
    assert(cash.collect());
    cash.update(1.0f, payment);
    assert(payment.change() == 0 && !cash.active());
    assert(!cash.returnChange(payment));
    animation.collect();
    // Sell the rest of a product, then verify sold-out cannot consume money.
    machine.select(0);
    for (int i = 0; i < productSlotCapacity; ++i) {
        payment.insert(50000);
        assert(machine.vendSelected(payment, animation));
        animation.update(10);
        assert(animation.delivered());
        animation.collect();
    }
    payment.insert(50000);
    assert(!machine.vendSelected(payment, animation));
    assert(payment.balance() == 50000 && machine.products()[0].stock == 0);
    assert(!animation.active());
    // Picking keypad and hovering beverages from a front-facing interaction camera.
    Camera camera;
    for (int i = 0; i < 100; ++i) camera.update(0.01f, 1, 0, false);
    assert(camera.beginInteraction());
    for (int i = 0; i < 100; ++i) camera.update(0.01f, 0, 0, false);
    const auto vp = camera.projectionMatrix(1.1f) * camera.viewMatrix();
    auto screen = [&](glm::vec3 p) {
        auto clip = vp * glm::vec4(p, 1);
        return glm::vec2((clip.x / clip.w + 1) * 550, (1 - clip.y / clip.w) * 500);
    };
    auto key = screen({1.68f, 3.48f, 0.78f});
    interaction.click(key.x, key.y, 1100, 1000, camera);
    assert(machine.selectedIndex() == 4 && animation.pressing());
    const auto product = screen(machine.productPosition(9));
    interaction.hover(product.x, product.y, 1100, 1000, camera);
    assert(machine.hoveredIndex() == 9);
    std::cout << "Selection, keypad ray casting, vending stages, stock, tray and change checks passed.\n";
}
