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
void Sound::cashMotor() {
#ifdef __APPLE__
    if (!queue_ || !buffer_) return;
    AudioQueueStop(queue_, true);
    AudioQueueReset(queue_);
    auto* samples = static_cast<std::int16_t*>(buffer_->mAudioData);
    for (int i = 0; i < 22050; ++i) {
        const double t = i / 22050.0;
        const double envelope = std::sin(3.14159265359 * t);
        const double motor = std::sin(6.28318530718 * 115 * t) * 0.55 +
                             std::sin(6.28318530718 * 233 * t) * 0.25 +
                             std::sin(6.28318530718 * 1500 * t) * std::exp(-t * 35) * 0.2;
        samples[i] = static_cast<std::int16_t>(motor * envelope * 6000);
    }
    buffer_->mAudioDataByteSize = 22050 * 2;
    AudioQueueEnqueueBuffer(queue_, buffer_, 0, nullptr);
    AudioQueueStart(queue_, nullptr);
#endif
}
