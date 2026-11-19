#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>
#include "KickEngine.h"

class SamuraiProcessor : public juce::AudioProcessor,
                         private juce::AudioProcessorValueTreeState::Listener,
                         private juce::Timer
{
public:
    SamuraiProcessor();
    ~SamuraiProcessor() override;

    //== AudioProcessor ========================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "THE SAMURAI"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //== Kick designer API (message thread) ====================================
    juce::AudioProcessorValueTreeState apvts;

    bool isSupportedFile (const juce::File& f) const;
    juce::String getSupportedWildcard() const;
    juce::String loadSource (int slot, const juce::File& file);   // returns error text, empty on success
    void clearSource (int slot);
    juce::String getSourceName (int slot) const;
    bool isSourceLoaded (int slot) const;

    void renderNow();
    int getRenderVersion() const { return renderVersion.load(); }
    juce::AudioBuffer<float> getRenderedCopy() const;
    double getRenderSampleRate() const { return currentSR.load(); }

    bool exportKick (const juce::File& file);
    juce::File exportTempKick();

    void explore();
    bool canGoBack() const { return backState.isValid(); }
    void goBack();

    bool savePreset (const juce::File& file);
    bool loadPreset (const juce::File& file);

    void triggerPreview() { previewRequested.store (true); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void parameterChanged (const juce::String&, float) override { dirty.store (true); }
    void timerCallback() override;

    KickSettings readSettings() const;
    float raw (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }
    void setParam (const juce::String& id, float plainValue);

    std::unique_ptr<juce::XmlElement> createStateXml();
    void restoreStateXml (const juce::XmlElement& xml);
    bool decodeAudio (juce::InputStream* stream, SourceSample& dest, bool trimAndNormalise);

    juce::AudioFormatManager formatManager;

    std::array<SourceSample, ParamIDs::numSources> sources;
    juce::CriticalSection sourceLock;
    juce::CriticalSection renderBuildLock;

    juce::AudioBuffer<float> rendered;   // read by the audio thread
    juce::AudioBuffer<float> uiCopy;     // message-thread copy for UI / export
    juce::SpinLock renderLock;
    std::atomic<int> renderVersion { 0 };
    std::atomic<bool> dirty { true };
    std::atomic<double> currentSR { 44100.0 };
    std::atomic<bool> previewRequested { false };

    juce::ValueTree backState;

    struct Voice
    {
        bool active = false;
        double pos = 0.0, rate = 1.0;
        float gain = 1.0f;
        int fadeLeft = -1;
    };
    std::array<Voice, 4> voices;
    static constexpr int fadeLen = 96;

    std::atomic<float>* keyTrackParam = nullptr;
    std::atomic<float>* velSensParam  = nullptr;

    void startVoice (int note, float velocity, bool ignoreKeyTrack);
    void renderVoices (juce::AudioBuffer<float>& buffer, int start, int end);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamuraiProcessor)
};
