#pragma once

class Animation;
class Camera;
class Payment;
class VendingMachine;

class Interaction {
public:
    Interaction(VendingMachine& machine, Payment& payment, Animation& animation);
    void click(float cursorX, float cursorY, int width, int height, const Camera& camera);
    void selectNext(int direction);
    bool purchase();

private:
    VendingMachine& machine_;
    Payment& payment_;
    Animation& animation_;
};
