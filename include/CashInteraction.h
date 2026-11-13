#pragma once

class Payment;

// Timed cash workflow. Credit is committed only after the bill has entered the slot.
class CashInteraction {
public:
    enum class State { Idle, Held, Inserting, Returning, AwaitingCollection, Collecting };
    bool hold(int denomination);
    bool insert();
    bool cancel(const Payment& payment);
    bool returnChange(const Payment& payment);
    bool collect();
    bool isChange() const { return returnChange_; }
    void update(float dt, Payment& payment);
    State state() const { return state_; }
    bool active() const { return state_ != State::Idle; }
    bool busy() const { return state_ == State::Inserting || state_ == State::Returning || state_ == State::Collecting; }
    int amount() const { return amount_; }
    float progress() const;
    bool takeSound();
private:
    State state_{State::Idle};
    int amount_{};
    float elapsed_{};
    bool sound_{};
    bool refundBalance_{};
    bool returnChange_{};
};
