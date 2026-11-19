#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace ParamIDs
{
    constexpr int numSources = 4;
    constexpr int numParts   = 3;
    constexpr int numStages  = 4;

    inline const char* partKey (int p)
    {
        static const char* keys[] = { "punch", "body", "tail" };
        return keys[p];
    }

    inline const char* partTitle (int p)
    {
        static const char* titles[] = { "PUNCH", "BODY", "TAIL" };
        return titles[p];
    }

    inline juce::String src  (int i, const char* what) { return "src" + juce::String (i) + "_" + what; }
    inline juce::String part (int p, const char* what) { return juce::String (partKey (p)) + "_" + what; }
    inline juce::String stage (int s)                  { return "stage" + juce::String (s + 1); }

    inline juce::StringArray sourceChoices()  { return { "A", "B", "C", "D" }; }
    inline juce::StringArray sourceChoicesB() { return { "None", "A", "B", "C", "D" }; }
    inline juce::StringArray stageChoices()   { return { "Off", "Impact", "Drive", "Tone", "Filter" }; }

    inline const char* punchEnd  = "punchEnd";
    inline const char* bodyEnd   = "bodyEnd";
    inline const char* xfade     = "xfade";

    inline const char* impact    = "impact";
    inline const char* drive     = "drive";
    inline const char* tone      = "tone";
    inline const char* filterCut = "filterCut";
    inline const char* filterRes = "filterRes";

    inline const char* eqLow     = "eqLow";
    inline const char* eqMid1F   = "eqMid1F";
    inline const char* eqMid1G   = "eqMid1G";
    inline const char* eqMid2F   = "eqMid2F";
    inline const char* eqMid2G   = "eqMid2G";
    inline const char* eqHigh    = "eqHigh";

    inline const char* length    = "length";
    inline const char* fade      = "fade";
    inline const char* tune      = "tune";
    inline const char* monoOn    = "monoOn";
    inline const char* monoFreq  = "monoFreq";
    inline const char* outGain   = "outGain";
    inline const char* velSens   = "velSens";
    inline const char* keyTrack  = "keyTrack";
}
