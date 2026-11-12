#pragma once

class Payment {
public:
    void insert(float amount);
    bool pay(float amount);
    void refund();
    float balance() const;
    float change() const;

private:
    float balance_{};
    float change_{};
};
