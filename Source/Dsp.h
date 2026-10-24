#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace tb
{
// Zero-delay-feedback (TPT) state variable filter, per-sample modulatable.
struct SVFCoeffs
{
    float a1 = 0.f, a2 = 0.f, a3 = 0.f, k = 1.f;

    void set (float fc, float q, float sampleRate)
    {
        fc = std::min (std::max (fc, 10.0f), 0.45f * sampleRate);
        q  = std::max (q, 0.1f);
        const float g = std::tan (3.14159265358979f * fc / sampleRate);
        k  = 1.0f / q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
};

struct SVFState
{
    float ic1 = 0.f, ic2 = 0.f;

    void reset() { ic1 = ic2 = 0.f; }

    inline void process (float x, const SVFCoeffs& c, float& highpass, float& bandpass)
    {
        const float v3 = x - ic2;
        const float v1 = c.a1 * ic1 + c.a2 * v3;
        const float v2 = ic2 + c.a2 * ic1 + c.a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        bandpass = v1;
        highpass = x - c.k * v1 - v2;
    }
};

// xorshift32 white noise in [-1, 1)
inline float nextNoise (uint32_t& s)
{
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return (float) (int32_t) s * (1.0f / 2147483648.0f);
}

inline float smoothstep (float e0, float e1, float x)
{
    const float t = std::min (std::max ((x - e0) / (e1 - e0), 0.0f), 1.0f);
    return t * t * (3.0f - 2.0f * t);
}
} // namespace tb
