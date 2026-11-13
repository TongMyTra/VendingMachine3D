#include "CashInteraction.h"
#include "Payment.h"
#include <cassert>
#include <iostream>

int main() {
    Payment payment;
    CashInteraction cash;
    assert(!cash.hold(15000));
    assert(!cash.insert());
    assert(cash.hold(10000));
    assert(!cash.hold(20000));
    assert(payment.balance() == 0);
    assert(cash.insert());
    assert(cash.takeSound());
    assert(!cash.takeSound());
    assert(!cash.insert());
    assert(!cash.cancel(payment));
    cash.update(1.5f, payment);
    assert(payment.balance() == 0);
    cash.update(0.5f, payment);
    assert(payment.balance() == 10000);
    cash.update(10.0f, payment);
    assert(payment.balance() == 10000); // Exactly once.
    for (int value : {20000, 50000}) {
        assert(cash.hold(value));
        assert(cash.insert());
        cash.update(2.0f, payment);
    }
    assert(payment.balance() == 80000);
    assert(cash.cancel(payment));
    assert(cash.amount() == 80000);
    cash.update(0.9f, payment);
    assert(payment.balance() == 80000); // Refund waits for animation.
    assert(!cash.hold(50000));
    cash.update(0.9f, payment);
    assert(payment.balance() == 0);
    assert(payment.change() == 80000);
    assert(!cash.active());
    assert(!cash.cancel(payment));
    assert(cash.hold(50000));
    assert(cash.cancel(payment)); // Uninserted note returns without creating credit.
    cash.update(2.0f, payment);
    assert(payment.balance() == 0);
    assert(cash.hold(20000));
    assert(cash.insert());
    cash.update(2.0f, payment);
    assert(payment.balance() == 20000);
    assert(payment.change() == 0);
    assert(!payment.pay(25000));
    assert(payment.pay(15000));
    assert(payment.change() == 5000);
    std::cout << "Cash deposit, duplicate guards, repeated bills, cancellation and refund passed.\n";
}
