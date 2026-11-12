#include "Payment.h"

void Payment::insert(float amount) {
    if (amount > 0.0f) {
        balance_ += amount;
        change_ = 0.0f;
    }
}

bool Payment::pay(float amount) {
    if (amount < 0.0f || balance_ + 0.001f < amount) {
        return false;
    }
    change_ = balance_ - amount;
    balance_ = 0.0f;
    return true;
}

void Payment::refund() {
    change_ = balance_;
    balance_ = 0.0f;
}

float Payment::balance() const {
    return balance_;
}

float Payment::change() const {
    return change_;
}
