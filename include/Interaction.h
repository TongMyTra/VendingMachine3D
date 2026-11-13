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
    void select(int index);
    void hover(float x, float y, int width, int height, const Camera& camera);
    bool purchase();

private:
    VendingMachine& machine_;
    Payment& payment_;
    Animation& animation_;
};
