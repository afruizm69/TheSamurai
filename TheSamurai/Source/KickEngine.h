#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include "Params.h"

// One loaded source kick (always stored as stereo at its native sample rate).
struct SourceSample
{
    juce::AudioBuffer<float> audio;
    double sampleRate = 44100.0;
    juce::String name;
    juce::String path;

    bool isLoaded() const { return audio.getNumSamples() > 0; }
    void clear() { audio.setSize (0, 0); name = {}; path = {}; }
};

struct PartSettings
{
    int   srcA    = 0;    // 0..3
    int   srcB    = -1;   // -1 = none, 0..3
    float blend   = 0.0f; // 0 = all A, 1 = all B
    float levelDb = 0.0f;
    float hpHz    = 20.0f;
    float lpHz    = 20000.0f;
    bool  solo    = false;
};

struct KickSettings
{
    std::array<float, ParamIDs::numSources> srcGainDb   {};
    std::array<float, ParamIDs::numSources> srcOffsetMs {};
    std::array<bool,  ParamIDs::numSources> srcInvert   {};

    float punchEndMs = 30.0f, bodyEndMs = 180.0f, xfadeMs = 6.0f;
    std::array<PartSettings, ParamIDs::numParts> parts;

    std::array<int, ParamIDs::numStages> stages { 1, 2, 3, 4 }; // 0 off, 1 impact, 2 drive, 3 tone, 4 filter
    float impact = 0.0f, drive = 0.0f, tone = 0.0f, filterCut = 20000.0f, filterRes = 0.707f;

    float eqLow = 0.0f, eqMid1F = 250.0f, eqMid1G = 0.0f, eqMid2F = 3000.0f, eqMid2G = 0.0f, eqHigh = 0.0f;

    float lengthMs = 800.0f, fadeMs = 60.0f, tune = 0.0f, monoFreq = 100.0f, outDb = 0.0f;
    bool  monoOn = true;
};

namespace KickEngine
{
    // Offline render of the full kick at the given sample rate. Called on the message thread.
    juce::AudioBuffer<float> render (const KickSettings& settings,
                                     const std::array<SourceSample, ParamIDs::numSources>& sources,
                                     double sampleRate);
}
