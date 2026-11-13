#pragma once
#ifdef __APPLE__
#include <AudioToolbox/AudioToolbox.h>
#endif

class Sound {
public:
    Sound();
    ~Sound();
    void cashMotor();
private:
#ifdef __APPLE__
    AudioQueueRef queue_{};
    AudioQueueBufferRef buffer_{};
#endif
};
