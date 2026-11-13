#include "Sound.h"
#include <cmath>
#include <cstdint>

Sound::Sound() {
#ifdef __APPLE__
    AudioStreamBasicDescription format{};
    format.mSampleRate = 22050;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
    format.mBytesPerPacket = format.mBytesPerFrame = 2;
    format.mFramesPerPacket = 1;
    format.mChannelsPerFrame = 1;
    format.mBitsPerChannel = 16;
    if (AudioQueueNewOutput(&format, [](void*, AudioQueueRef, AudioQueueBufferRef) {},
                            nullptr, nullptr, nullptr, 0, &queue_) != noErr) queue_ = nullptr;
    if (queue_ && AudioQueueAllocateBuffer(queue_, 22050 * 2, &buffer_) != noErr) {
        AudioQueueDispose(queue_, true);
        queue_ = nullptr;
    }
#endif
}
Sound::~Sound() {
#ifdef __APPLE__
    if (queue_) AudioQueueDispose(queue_, true);
#endif
}
void Sound::cashMotor() { effect(2); }
void Sound::effect(int kind) {
#ifdef __APPLE__
    if (!queue_ || !buffer_) return;
    AudioQueueStop(queue_, true);
    AudioQueueReset(queue_);
    auto* samples = static_cast<std::int16_t*>(buffer_->mAudioData);
    const int count = kind == 1 ? 2600 : kind == 8 ? 11025 : kind >= 3 ? 7000 : 22050;
    for (int i = 0; i < count; ++i) {
        const double t = i / 22050.0;
        const double envelope = kind == 2 ? std::sin(3.14159265359 * t) : std::exp(-t * 20);
        double motor = std::sin(6.28318530718 * 115 * t) * 0.55 +
                             std::sin(6.28318530718 * 233 * t) * 0.25 +
                             std::sin(6.28318530718 * 1500 * t) * std::exp(-t * 35) * 0.2;
        if (kind != 2) {
            const double frequency = kind == 1 ? 900 : kind == 5 ? 260 : 110;
            motor = std::sin(6.28318530718 * frequency * t) * 0.7 +
                    std::sin(6.28318530718 * frequency * 2.73 * t) * 0.3;
        }
        if (kind == 6) {
            // Short metallic pop followed by a fading hiss.
            const double noise = std::sin(i * 78.233) * std::sin(i * 12.9898);
            motor = (std::sin(6.28318530718 * 2100 * t) * std::exp(-t * 70) + noise * std::exp(-t * 14));
        } else if (kind == 7) {
            motor = std::sin(6.28318530718 * 620 * t) * std::sin(6.28318530718 * 25 * t);
        } else if (kind == 8) {
            motor = std::sin(6.28318530718 * (160 * t + 35 * t * t)) * (0.5 + 0.5 * std::sin(6.28318530718 * 9 * t));
        }
        samples[i] = static_cast<std::int16_t>(motor * envelope * 6000);
    }
    buffer_->mAudioDataByteSize = count * 2;
    AudioQueueEnqueueBuffer(queue_, buffer_, 0, nullptr);
    AudioQueueStart(queue_, nullptr);
#else
    (void)kind;
#endif
}
