#include "CashInteraction.h"
#include "Payment.h"
#include <algorithm>
#include <cmath>

bool CashInteraction::hold(int amount) {
    if (active() || (amount != 10000 && amount != 20000 && amount != 50000)) return false;
    amount_ = amount;
    elapsed_ = 0.0f;
    state_ = State::Held;
    return true;
}
bool CashInteraction::insert() {
    if (state_ != State::Held) return false;
    state_ = State::Inserting;
    elapsed_ = 0.0f;
    sound_ = true;
    return true;
}
bool CashInteraction::cancel(const Payment& payment) {
    if (busy()) return false;
    refundBalance_ = payment.balance() > 0.0f;
    if (!active() && !refundBalance_) return false;
    amount_ = refundBalance_ ? static_cast<int>(std::lround(payment.balance())) : amount_;
    state_ = State::Returning;
    elapsed_ = 0.0f;
    sound_ = true;
    return true;
}
float CashInteraction::progress() const {
    return std::clamp(elapsed_ / (state_ == State::Returning ? 1.8f : 2.0f), 0.0f, 1.0f);
}
void CashInteraction::update(float dt, Payment& payment) {
    if (!busy()) return;
    elapsed_ += std::max(dt, 0.0f);
    if (progress() < 1.0f) return;
    if (state_ == State::Inserting) payment.insert(static_cast<float>(amount_));
    else if (refundBalance_) payment.refund();
    state_ = State::Idle;
    amount_ = 0;
    refundBalance_ = false;
}
bool CashInteraction::takeSound() {
    const bool result = sound_;
    sound_ = false;
    return result;
}
