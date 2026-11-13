#include "HeldDrink.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    const auto products = createProducts();
    HeldDrink drink;
    assert(!drink.open());
    assert(!drink.discardEmpty());
    assert(drink.pickUp(products[0], {-0.43f, 0.49f, 1.17f}));
    assert(!drink.pickUp(products[1], {}));
    assert(drink.pickingUp() && !drink.open());
    drink.update(1, true);
    assert(!drink.pickingUp() && drink.remaining() == 330);
    assert(drink.open());
    assert(!drink.open());
    drink.update(0.8f, true);
    assert(drink.opening() && !drink.opened());
    assert(drink.remaining() == 330 && drink.takeSound() == 6);
    drink.update(0.8f, false);
    assert(drink.opened() && !drink.opening() && !drink.open());
    assert(drink.openingProgress() == 1);
    for (int i = 0; i < 100; ++i) drink.update(0.01f, true);
    assert(drink.remaining() < 330 && drink.remaining() > 270);
    const auto remaining = drink.remaining();
    const float raised = drink.drinkBlend();
    drink.update(0.01f, false);
    assert(drink.drinkBlend() < raised && drink.drinkBlend() > 0.5f);
    for (int i = 0; i < 100; ++i) drink.update(0.01f, false);
    assert(drink.remaining() == remaining && drink.drinkBlend() < 0.01f);
    for (int i = 0; i < 1000; ++i) drink.update(0.01f, true);
    assert(drink.empty() && drink.remaining() == 0);
    for (int i = 0; i < 200; ++i) drink.update(0.01f, true);
    assert(drink.remaining() == 0 && drink.drinkBlend() < 0.01f);
    assert(drink.discardEmpty() && !drink.holding());
    assert(drink.pickUp(products[4], {}));
    assert(drink.capacity() == 500 && !drink.opened());
    drink.update(1, false);
    assert(drink.open());
    drink.update(1.1f, false);
    assert(drink.takeSound() == 7 && drink.opening());
    drink.update(1.1f, false);
    assert(drink.opened() && !drink.open());
    assert(drink.remaining() == 500 && !drink.discardEmpty());
    assert(drink.toggleInspect());
    drink.inspectRotate(900, 1000);
    drink.inspectZoom(100);
    assert(std::isfinite(drink.inspectYaw()) && drink.inspectPitch() <= 85);
    assert(drink.inspectDistance() >= 0.65f);
    drink.update(0.5f, true);
    assert(drink.remaining() == 500); // Inspection cannot consume contents.
    assert(!drink.putDown({4.8f, 2.49f, 0.8f}));
    assert(drink.toggleInspect());
    drink.update(1, false);
    for (int i = 0; i < 100; ++i) drink.update(0.01f, true);
    drink.update(1, false);
    const float savedVolume = drink.remaining();
    assert(savedVolume < 500);
    assert(drink.putDown({4.8f, 2.49f, 0.8f}));
    assert(!drink.holding() && drink.placed() && drink.hasProduct());
    assert(!drink.takePlaced()); // Wait for placing animation.
    assert(!drink.pickUp(products[0], {})); // Do not replace a placed product.
    drink.update(1, true);
    assert(drink.remaining() == savedVolume && drink.opened());
    assert(drink.takePlaced() && drink.pickupFromTable());
    drink.update(1, false);
    assert(drink.holding() && drink.opened() && !drink.open());
    assert(drink.remaining() == savedVolume);
    assert(!drink.throwAway({-4.9f, 1.0f, 0.4f})); // Only empty containers go in trash.
    for (int i = 0; i < 2000; ++i) drink.update(0.01f, true);
    assert(drink.empty());
    assert(drink.throwAway({-4.9f, 1.0f, 0.4f}));
    assert(drink.tossing() && !drink.holding());
    drink.update(0.4f, false);
    assert(drink.hasProduct());
    drink.update(0.6f, false);
    assert(!drink.hasProduct() && !drink.tossing());
    std::cout << "Pickup, can/bottle opening, persistent state, sipping, release and empty guards passed.\n";
}
