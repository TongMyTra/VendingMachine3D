#pragma once

class Animation {
public:
    void start(int productIndex);
    void update(float deltaTime);
    bool active() const;
    int productIndex() const;
    float dropOffset() const;
    float progress() const;

private:
    float elapsed_{};
    int productIndex_{-1};
    bool active_{};
};
