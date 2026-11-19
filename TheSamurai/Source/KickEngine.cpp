#include "KickEngine.h"
#include <cmath>

namespace
{
    constexpr double kPi = 3.14159265358979323846;

    float hermite (const float* d, int n, double pos)
    {
        const int i = (int) std::floor (pos);
        const float f = (float) (pos - (double) i);
        auto at = [d, n] (int k) { return (k >= 0 && k < n) ? d[k] : 0.0f; };

        const float xm1 = at (i - 1), x0 = at (i), x1 = at (i + 1), x2 = at (i + 2);
        const float c0 = x0;
        const float c1 = 0.5f * (x1 - xm1);
        const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * f + c2) * f + c1) * f + c0;
    }

    double clampFreq (double f, double sr) { return juce::jlimit (10.0, sr * 0.45, f); }

    void applyIIR (juce::AudioBuffer<float>& b, const juce::IIRCoefficients& c)
    {
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
        {
            juce::IIRFilter f;
            f.setCoefficients (c);
            f.processSamples (b.getWritePointer (ch), b.getNumSamples());
        }
    }

    // Forward-backward filtering: zero phase, used where phase shift would hurt the low end.
    void applyIIRZeroPhase (juce::AudioBuffer<float>& b, const juce::IIRCoefficients& c)
    {
        const int n = b.getNumSamples();
        applyIIR (b, c);
        for (int ch = 0; ch < b.getNumChannels(); ++ch) b.reverse (ch, 0, n);
        applyIIR (b, c);
        for (int ch = 0; ch < b.getNumChannels(); ++ch) b.reverse (ch, 0, n);
    }

    void applyImpact (juce::AudioBuffer<float>& b, float amount, double sr)
    {
        if (amount <= 0.001f) return;

        const int n = b.getNumSamples();
        auto coef = [sr] (double seconds) { return (float) std::exp (-1.0 / (seconds * sr)); };
        const float aF = coef (0.0002), rF = coef (0.005);
        const float aS = coef (0.015),  rS = coef (0.060);
        float ef = 0.0f, es = 0.0f;

        auto* L = b.getWritePointer (0);
        auto* R = b.getWritePointer (1);

        for (int i = 0; i < n; ++i)
        {
            const float x = juce::jmax (std::abs (L[i]), std::abs (R[i]));
            ef = x > ef ? aF * ef + (1.0f - aF) * x : rF * ef + (1.0f - rF) * x;
            es = x > es ? aS * es + (1.0f - aS) * x : rS * es + (1.0f - rS) * x;

            const float ratio = (ef + 1.0e-5f) / (es + 1.0e-5f);
            const float g = juce::jlimit (0.25f, 4.0f, std::pow (ratio, amount * 1.5f));
            L[i] *= g;
            R[i] *= g;
        }
    }

    void applyDrive (juce::AudioBuffer<float>& b, float drive)
    {
        if (drive <= 0.001f) return;

        const float g = 1.0f + 30.0f * drive * drive;
        const float norm = 1.0f / std::tanh (g);
        for (int ch = 0; ch < b.getNumChannels(); ++ch)
        {
            auto* d = b.getWritePointer (ch);
            for (int i = 0; i < b.getNumSamples(); ++i)
                d[i] = std::tanh (g * d[i]) * norm;
        }
    }

    void applyTone (juce::AudioBuffer<float>& b, float tone, double sr)
    {
        if (std::abs (tone) < 0.01f) return;
        const float db = tone * 6.0f;
        applyIIR (b, juce::IIRCoefficients::makeLowShelf  (sr, clampFreq (300.0, sr),  0.707, juce::Decibels::decibelsToGain (-db)));
        applyIIR (b, juce::IIRCoefficients::makeHighShelf (sr, clampFreq (3000.0, sr), 0.707, juce::Decibels::decibelsToGain (db)));
    }

    void applyFilter (juce::AudioBuffer<float>& b, float cut, float q, double sr)
    {
        if (cut >= 19900.0f) return;
        applyIIR (b, juce::IIRCoefficients::makeLowPass (sr, clampFreq (cut, sr), juce::jmax (0.1f, q)));
    }

    void applyEQ (juce::AudioBuffer<float>& b, const KickSettings& s, double sr)
    {
        auto g = [] (float db) { return juce::Decibels::decibelsToGain (db); };
        if (std::abs (s.eqLow)   > 0.05f) applyIIR (b, juce::IIRCoefficients::makeLowShelf  (sr, clampFreq (60.0, sr),       0.707, g (s.eqLow)));
        if (std::abs (s.eqMid1G) > 0.05f) applyIIR (b, juce::IIRCoefficients::makePeakFilter (sr, clampFreq (s.eqMid1F, sr), 1.2,   g (s.eqMid1G)));
        if (std::abs (s.eqMid2G) > 0.05f) applyIIR (b, juce::IIRCoefficients::makePeakFilter (sr, clampFreq (s.eqMid2F, sr), 1.2,   g (s.eqMid2G)));
        if (std::abs (s.eqHigh)  > 0.05f) applyIIR (b, juce::IIRCoefficients::makeHighShelf (sr, clampFreq (8000.0, sr),     0.707, g (s.eqHigh)));
    }

    // Keeps everything below 'freq' mono: high-passes only the side signal (zero phase).
    void applyMonoBass (juce::AudioBuffer<float>& b, float freq, double sr)
    {
        const int n = b.getNumSamples();
        juce::AudioBuffer<float> side (1, n);
        auto* L = b.getWritePointer (0);
        auto* R = b.getWritePointer (1);
        auto* S = side.getWritePointer (0);

        for (int i = 0; i < n; ++i) S[i] = 0.5f * (L[i] - R[i]);

        applyIIRZeroPhase (side, juce::IIRCoefficients::makeHighPass (sr, clampFreq (freq, sr), 0.707));

        for (int i = 0; i < n; ++i)
        {
            const float m = 0.5f * (L[i] + R[i]);
            L[i] = m + S[i];
            R[i] = m - S[i];
        }
    }
}

juce::AudioBuffer<float> KickEngine::render (const KickSettings& s,
                                             const std::array<SourceSample, ParamIDs::numSources>& src,
                                             double sr)
{
    const int N = juce::jmax (16, (int) std::round (s.lengthMs * 0.001 * sr));
    juce::AudioBuffer<float> out (2, N);
    out.clear();

    const double pitch = std::pow (2.0, s.tune / 12.0);

    bool anySolo = false;
    for (auto& p : s.parts) anySolo = anySolo || p.solo;

    auto partActive = [&] (const PartSettings& p) { return ! anySolo || p.solo; };

    // 1. Prepare (resample, offset, gain, polarity) only the sources actually used.
    std::array<bool, ParamIDs::numSources> used {};
    for (auto& p : s.parts)
    {
        if (! partActive (p)) continue;
        used[(size_t) juce::jlimit (0, 3, p.srcA)] = true;
        if (p.srcB >= 0 && p.blend > 0.0f) used[(size_t) juce::jlimit (0, 3, p.srcB)] = true;
    }

    std::array<juce::AudioBuffer<float>, ParamIDs::numSources> prep;
    for (size_t i = 0; i < (size_t) ParamIDs::numSources; ++i)
    {
        if (! used[i] || ! src[i].isLoaded()) continue;

        const auto& a = src[i].audio;
        const int len = a.getNumSamples();
        const double ratio = src[i].sampleRate / sr * pitch;
        const double off = s.srcOffsetMs[i] * 0.001 * sr;
        const float gain = juce::Decibels::decibelsToGain (s.srcGainDb[i]) * (s.srcInvert[i] ? -1.0f : 1.0f);

        prep[i].setSize (2, N);
        for (int ch = 0; ch < 2; ++ch)
        {
            const float* d = a.getReadPointer (juce::jmin (ch, a.getNumChannels() - 1));
            float* o = prep[i].getWritePointer (ch);
            for (int n = 0; n < N; ++n)
            {
                const double t = ((double) n - off) * ratio;
                o[n] = (t < -2.0 || t > (double) len + 2.0) ? 0.0f : hermite (d, len, t) * gain;
            }
        }
    }

    // 2. Build each part (Punch / Body / Tail) and window it into the output.
    const double xf = juce::jmax (1.0, s.xfadeMs * 0.001 * sr);
    const double b1 = s.punchEndMs * 0.001 * sr;
    const double b2 = juce::jmax ((double) s.bodyEndMs * 0.001 * sr, b1 + xf);

    auto ramp = [xf] (double t, double c)
    {
        const double x = juce::jlimit (0.0, 1.0, (t - (c - xf * 0.5)) / xf);
        const double sn = std::sin (x * kPi * 0.5);
        return sn * sn;
    };

    juce::AudioBuffer<float> part (2, N);

    for (int p = 0; p < ParamIDs::numParts; ++p)
    {
        const auto& ps = s.parts[(size_t) p];
        if (! partActive (ps)) continue;

        part.clear();
        const int a = juce::jlimit (0, 3, ps.srcA);
        const bool hasB = ps.srcB >= 0;
        const float wA = hasB ? 1.0f - ps.blend : 1.0f;

        if (prep[(size_t) a].getNumSamples() == N)
            for (int ch = 0; ch < 2; ++ch) part.addFrom (ch, 0, prep[(size_t) a], ch, 0, N, wA);

        if (hasB)
        {
            const int bIdx = juce::jlimit (0, 3, ps.srcB);
            if (prep[(size_t) bIdx].getNumSamples() == N)
                for (int ch = 0; ch < 2; ++ch) part.addFrom (ch, 0, prep[(size_t) bIdx], ch, 0, N, ps.blend);
        }

        if (ps.hpHz > 20.5f)    applyIIR (part, juce::IIRCoefficients::makeHighPass (sr, clampFreq (ps.hpHz, sr), 0.707));
        if (ps.lpHz < 19900.0f) applyIIR (part, juce::IIRCoefficients::makeLowPass  (sr, clampFreq (ps.lpHz, sr), 0.707));

        const float lvl = juce::Decibels::decibelsToGain (ps.levelDb);

        for (int ch = 0; ch < 2; ++ch)
        {
            const float* pd = part.getReadPointer (ch);
            float* od = out.getWritePointer (ch);
            for (int n = 0; n < N; ++n)
            {
                const double t = (double) n;
                double w;
                if (p == 0)      w = 1.0 - ramp (t, b1);
                else if (p == 1) w = ramp (t, b1) * (1.0 - ramp (t, b2));
                else             w = ramp (t, b2);
                od[n] += pd[n] * lvl * (float) w;
            }
        }
    }

    // 3. Sculpt chain in the user's chosen order.
    for (int st : s.stages)
    {
        switch (st)
        {
            case 1: applyImpact (out, s.impact, sr); break;
            case 2: applyDrive  (out, s.drive); break;
            case 3: applyTone   (out, s.tone, sr); break;
            case 4: applyFilter (out, s.filterCut, s.filterRes, sr); break;
            default: break;
        }
    }

    // 4. EQ, mono low end, fade out, output gain.
    applyEQ (out, s, sr);

    if (s.monoOn)
        applyMonoBass (out, s.monoFreq, sr);

    const int fadeN = juce::jlimit (1, N, (int) std::round (s.fadeMs * 0.001 * sr));
    for (int ch = 0; ch < 2; ++ch)
    {
        float* d = out.getWritePointer (ch);
        for (int i = 0; i < fadeN; ++i)
        {
            const double x = (double) i / (double) fadeN;            // 0 -> 1 across the fade
            d[N - fadeN + i] *= (float) (0.5 * (1.0 + std::cos (x * kPi)));
        }
    }

    out.applyGain (juce::Decibels::decibelsToGain (s.outDb));
    return out;
}
