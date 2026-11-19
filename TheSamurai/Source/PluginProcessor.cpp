#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace ParamIDs;
using APVTS = juce::AudioProcessorValueTreeState;

//==============================================================================
namespace
{
    juce::NormalisableRange<float> skewed (float lo, float hi, float centre)
    {
        juce::NormalisableRange<float> r (lo, hi);
        r.setSkewForCentre (centre);
        return r;
    }

    juce::AudioParameterFloatAttributes fmt (const juce::String& suffix, int decimals)
    {
        return juce::AudioParameterFloatAttributes().withStringFromValueFunction (
            [suffix, decimals] (float v, int) { return juce::String (v, decimals) + suffix; });
    }

    juce::AudioParameterFloatAttributes hzFmt()
    {
        return juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
        {
            return v >= 1000.0f ? juce::String (v / 1000.0f, 1) + " kHz" : juce::String ((int) v) + " Hz";
        });
    }

    juce::AudioParameterFloatAttributes pctFmt()
    {
        return juce::AudioParameterFloatAttributes().withStringFromValueFunction (
            [] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; });
    }
}

APVTS::ParameterLayout SamuraiProcessor::createLayout()
{
    using F = juce::AudioParameterFloat;
    using B = juce::AudioParameterBool;
    using C = juce::AudioParameterChoice;
    using PID = juce::ParameterID;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    const char* letters[] = { "A", "B", "C", "D" };

    for (int i = 0; i < numSources; ++i)
    {
        const juce::String L = letters[i];
        p.push_back (std::make_unique<F> (PID { src (i, "gain"), 1 },   "Source " + L + " Gain",   juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f, fmt (" dB", 1)));
        p.push_back (std::make_unique<F> (PID { src (i, "offset"), 1 }, "Source " + L + " Offset", juce::NormalisableRange<float> (-10.0f, 10.0f, 0.01f), 0.0f, fmt (" ms", 2)));
        p.push_back (std::make_unique<B> (PID { src (i, "invert"), 1 }, "Source " + L + " Invert", false));
    }

    p.push_back (std::make_unique<F> (PID { punchEnd, 1 }, "Punch End",  juce::NormalisableRange<float> (5.0f, 200.0f, 0.1f),  30.0f,  fmt (" ms", 1)));
    p.push_back (std::make_unique<F> (PID { bodyEnd, 1 },  "Body End",   juce::NormalisableRange<float> (20.0f, 1500.0f, 0.1f), 180.0f, fmt (" ms", 1)));
    p.push_back (std::make_unique<F> (PID { xfade, 1 },    "Crossfade",  juce::NormalisableRange<float> (0.5f, 40.0f, 0.1f),   6.0f,   fmt (" ms", 1)));

    const int defA[] = { 0, 1, 1 };
    for (int pt = 0; pt < numParts; ++pt)
    {
        const juce::String T = partTitle (pt);
        p.push_back (std::make_unique<C> (PID { part (pt, "srcA"), 1 },  T + " Source A", sourceChoices(),  defA[pt]));
        p.push_back (std::make_unique<C> (PID { part (pt, "srcB"), 1 },  T + " Source B", sourceChoicesB(), 0));
        p.push_back (std::make_unique<F> (PID { part (pt, "blend"), 1 }, T + " Blend",    juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f, pctFmt()));
        p.push_back (std::make_unique<F> (PID { part (pt, "level"), 1 }, T + " Level",    juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f, fmt (" dB", 1)));
        p.push_back (std::make_unique<F> (PID { part (pt, "hp"), 1 },    T + " Low Cut",  skewed (20.0f, 2000.0f, 150.0f), 20.0f, hzFmt()));
        p.push_back (std::make_unique<F> (PID { part (pt, "lp"), 1 },    T + " High Cut", skewed (200.0f, 20000.0f, 3000.0f), 20000.0f, hzFmt()));
        p.push_back (std::make_unique<B> (PID { part (pt, "solo"), 1 },  T + " Solo", false));
        p.push_back (std::make_unique<B> (PID { part (pt, "lock"), 1 },  T + " Lock", false));
    }

    for (int s = 0; s < numStages; ++s)
        p.push_back (std::make_unique<C> (PID { stage (s), 1 }, "Sculpt Stage " + juce::String (s + 1), stageChoices(), s + 1));

    p.push_back (std::make_unique<F> (PID { impact, 1 },    "Impact",     juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.25f, pctFmt()));
    p.push_back (std::make_unique<F> (PID { drive, 1 },     "Drive",      juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.15f, pctFmt()));
    p.push_back (std::make_unique<F> (PID { tone, 1 },      "Tone",       juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f, fmt ("", 2)));
    p.push_back (std::make_unique<F> (PID { filterCut, 1 }, "Filter",     skewed (40.0f, 20000.0f, 2000.0f), 20000.0f, hzFmt()));
    p.push_back (std::make_unique<F> (PID { filterRes, 1 }, "Resonance",  juce::NormalisableRange<float> (0.3f, 8.0f, 0.01f), 0.707f, fmt ("", 2)));

    p.push_back (std::make_unique<F> (PID { eqLow, 1 },   "EQ Low",        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, fmt (" dB", 1)));
    p.push_back (std::make_unique<F> (PID { eqMid1F, 1 }, "EQ Mid 1 Freq", skewed (80.0f, 1500.0f, 300.0f), 250.0f, hzFmt()));
    p.push_back (std::make_unique<F> (PID { eqMid1G, 1 }, "EQ Mid 1 Gain", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, fmt (" dB", 1)));
    p.push_back (std::make_unique<F> (PID { eqMid2F, 1 }, "EQ Mid 2 Freq", skewed (800.0f, 12000.0f, 3000.0f), 3000.0f, hzFmt()));
    p.push_back (std::make_unique<F> (PID { eqMid2G, 1 }, "EQ Mid 2 Gain", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, fmt (" dB", 1)));
    p.push_back (std::make_unique<F> (PID { eqHigh, 1 },  "EQ High",       juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, fmt (" dB", 1)));

    p.push_back (std::make_unique<F> (PID { length, 1 },   "Length",      juce::NormalisableRange<float> (100.0f, 3000.0f, 1.0f), 800.0f, fmt (" ms", 0)));
    p.push_back (std::make_unique<F> (PID { fade, 1 },     "Fade Out",    juce::NormalisableRange<float> (5.0f, 800.0f, 1.0f), 60.0f, fmt (" ms", 0)));
    p.push_back (std::make_unique<F> (PID { tune, 1 },     "Tune",        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.01f), 0.0f, fmt (" st", 2)));
    p.push_back (std::make_unique<B> (PID { monoOn, 1 },   "Mono Bass",   true));
    p.push_back (std::make_unique<F> (PID { monoFreq, 1 }, "Mono Below",  skewed (30.0f, 400.0f, 120.0f), 120.0f, hzFmt()));
    p.push_back (std::make_unique<F> (PID { outGain, 1 },  "Output",      juce::NormalisableRange<float> (-24.0f, 6.0f, 0.1f), 0.0f, fmt (" dB", 1)));
    p.push_back (std::make_unique<F> (PID { velSens, 1 },  "Velocity",    juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f, pctFmt()));
    p.push_back (std::make_unique<B> (PID { keyTrack, 1 }, "Key Track",   false));

    return { p.begin(), p.end() };
}

//==============================================================================
SamuraiProcessor::SamuraiProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    formatManager.registerBasicFormats();

    for (auto* param : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
            apvts.addParameterListener (rp->paramID, this);

    keyTrackParam = apvts.getRawParameterValue (ParamIDs::keyTrack);
    velSensParam  = apvts.getRawParameterValue (ParamIDs::velSens);

    renderNow();
    startTimerHz (30);
}

SamuraiProcessor::~SamuraiProcessor()
{
    stopTimer();
    for (auto* param : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
            apvts.removeParameterListener (rp->paramID, this);
}

bool SamuraiProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void SamuraiProcessor::prepareToPlay (double sampleRate, int)
{
    for (auto& v : voices) v.active = false;

    if (std::abs (sampleRate - currentSR.load()) > 0.5)
    {
        currentSR.store (sampleRate);
        renderNow();
    }
}

void SamuraiProcessor::timerCallback()
{
    if (dirty.exchange (false))
        renderNow();
}

//==============================================================================
KickSettings SamuraiProcessor::readSettings() const
{
    KickSettings s;

    for (int i = 0; i < numSources; ++i)
    {
        s.srcGainDb[(size_t) i]   = raw (src (i, "gain"));
        s.srcOffsetMs[(size_t) i] = raw (src (i, "offset"));
        s.srcInvert[(size_t) i]   = raw (src (i, "invert")) > 0.5f;
    }

    s.punchEndMs = raw (punchEnd);
    s.bodyEndMs  = raw (bodyEnd);
    s.xfadeMs    = raw (xfade);

    for (int pt = 0; pt < numParts; ++pt)
    {
        auto& ps = s.parts[(size_t) pt];
        ps.srcA    = (int) raw (part (pt, "srcA"));
        ps.srcB    = (int) raw (part (pt, "srcB")) - 1;
        ps.blend   = raw (part (pt, "blend"));
        ps.levelDb = raw (part (pt, "level"));
        ps.hpHz    = raw (part (pt, "hp"));
        ps.lpHz    = raw (part (pt, "lp"));
        ps.solo    = raw (part (pt, "solo")) > 0.5f;
    }

    for (int st = 0; st < numStages; ++st)
        s.stages[(size_t) st] = (int) raw (stage (st));

    s.impact    = raw (impact);
    s.drive     = raw (drive);
    s.tone      = raw (tone);
    s.filterCut = raw (filterCut);
    s.filterRes = raw (filterRes);

    s.eqLow   = raw (eqLow);
    s.eqMid1F = raw (eqMid1F);
    s.eqMid1G = raw (eqMid1G);
    s.eqMid2F = raw (eqMid2F);
    s.eqMid2G = raw (eqMid2G);
    s.eqHigh  = raw (eqHigh);

    s.lengthMs = raw (length);
    s.fadeMs   = raw (fade);
    s.tune     = raw (tune);
    s.monoOn   = raw (monoOn) > 0.5f;
    s.monoFreq = raw (monoFreq);
    s.outDb    = raw (outGain);
    return s;
}

void SamuraiProcessor::renderNow()
{
    const juce::ScopedLock buildLock (renderBuildLock);

    const auto settings = readSettings();
    juce::AudioBuffer<float> fresh;
    {
        const juce::ScopedLock sl (sourceLock);
        fresh = KickEngine::render (settings, sources, currentSR.load());
    }

    uiCopy.makeCopyOf (fresh);
    {
        const juce::SpinLock::ScopedLockType sl (renderLock);
        std::swap (rendered, fresh);
    }
    // 'fresh' now holds the old buffer and is freed here, off the audio thread.
    ++renderVersion;
}

juce::AudioBuffer<float> SamuraiProcessor::getRenderedCopy() const
{
    const juce::ScopedLock buildLock (renderBuildLock);
    juce::AudioBuffer<float> copy;
    copy.makeCopyOf (uiCopy);
    return copy;
}

//==============================================================================
void SamuraiProcessor::startVoice (int note, float velocity, bool ignoreKeyTrack)
{
    for (auto& v : voices)
        if (v.active && v.fadeLeft < 0)
            v.fadeLeft = fadeLen;

    Voice* target = nullptr;
    for (auto& v : voices)
        if (! v.active) { target = &v; break; }

    if (target == nullptr)
    {
        target = &voices[0];
        for (auto& v : voices)
            if (v.pos > target->pos) target = &v;
    }

    const bool track = ! ignoreKeyTrack && keyTrackParam->load() > 0.5f;
    const float sens = velSensParam->load();

    target->active   = true;
    target->pos      = 0.0;
    target->rate     = track ? std::pow (2.0, (note - 60) / 12.0) : 1.0;
    target->gain     = 1.0f - sens * (1.0f - velocity);
    target->fadeLeft = -1;
}

void SamuraiProcessor::renderVoices (juce::AudioBuffer<float>& buffer, int start, int end)
{
    const int len = rendered.getNumSamples();
    if (len < 2 || end <= start) return;

    const float* L = rendered.getReadPointer (0);
    const float* R = rendered.getReadPointer (1);
    const int outCh = buffer.getNumChannels();

    for (auto& v : voices)
    {
        if (! v.active) continue;

        for (int i = start; i < end; ++i)
        {
            const int idx = (int) v.pos;
            if (idx >= len - 1) { v.active = false; break; }

            float g = v.gain;
            if (v.fadeLeft >= 0)
            {
                if (v.fadeLeft == 0) { v.active = false; break; }
                g *= (float) v.fadeLeft / (float) fadeLen;
                --v.fadeLeft;
            }

            const float fr = (float) (v.pos - (double) idx);
            const float l = L[idx] + fr * (L[idx + 1] - L[idx]);
            const float r = R[idx] + fr * (R[idx + 1] - R[idx]);

            if (outCh >= 2)
            {
                buffer.addSample (0, i, l * g);
                buffer.addSample (1, i, r * g);
            }
            else if (outCh == 1)
            {
                buffer.addSample (0, i, 0.5f * (l + r) * g);
            }

            v.pos += v.rate;
        }
    }
}

void SamuraiProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const juce::SpinLock::ScopedTryLockType lock (renderLock);
    if (! lock.isLocked())
        return;

    const int numSamples = buffer.getNumSamples();

    if (previewRequested.exchange (false))
        startVoice (60, 1.0f, true);

    int pos = 0;
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        const int t = juce::jlimit (0, numSamples, meta.samplePosition);
        renderVoices (buffer, pos, t);
        pos = t;

        if (msg.isNoteOn())
            startVoice (msg.getNoteNumber(), msg.getFloatVelocity(), false);
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            for (auto& v : voices)
                if (v.active && v.fadeLeft < 0) v.fadeLeft = fadeLen;
    }

    renderVoices (buffer, pos, numSamples);
}

//==============================================================================
bool SamuraiProcessor::isSupportedFile (const juce::File& f) const
{
    return f.existsAsFile() && formatManager.findFormatForFileExtension (f.getFileExtension()) != nullptr;
}

juce::String SamuraiProcessor::getSupportedWildcard() const
{
    return formatManager.getWildcardForAllFormats();
}

bool SamuraiProcessor::decodeAudio (juce::InputStream* stream, SourceSample& dest, bool trimAndNormalise)
{
    if (stream == nullptr)
        return false;

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (std::unique_ptr<juce::InputStream> (stream)));
    if (reader == nullptr || reader->lengthInSamples <= 0 || reader->sampleRate <= 0)
        return false;

    const int maxLen = (int) (reader->sampleRate * 8.0);
    const int len = (int) juce::jmin ((juce::int64) maxLen, reader->lengthInSamples);

    juce::AudioBuffer<float> buf (2, len);
    buf.clear();
    reader->read (&buf, 0, len, 0, true, true);
    if (reader->numChannels == 1)
        buf.copyFrom (1, 0, buf, 0, 0, len);

    if (trimAndNormalise)
    {
        // Trim leading silence so all kicks start aligned.
        int first = 0;
        for (; first < len; ++first)
            if (std::abs (buf.getSample (0, first)) > 0.0005f || std::abs (buf.getSample (1, first)) > 0.0005f)
                break;
        first = juce::jmax (0, first - 2);

        if (first > 0 && first < len - 16)
        {
            juce::AudioBuffer<float> trimmed (2, len - first);
            for (int ch = 0; ch < 2; ++ch)
                trimmed.copyFrom (ch, 0, buf, ch, first, len - first);
            buf = std::move (trimmed);
        }

        // Normalise peak to -0.3 dBFS so blends are level-matched.
        const float peak = buf.getMagnitude (0, buf.getNumSamples());
        if (peak > 1.0e-4f)
            buf.applyGain (juce::Decibels::decibelsToGain (-0.3f) / peak);
    }

    dest.audio = std::move (buf);
    dest.sampleRate = reader->sampleRate;
    return true;
}

juce::String SamuraiProcessor::loadSource (int slot, const juce::File& file)
{
    if (slot < 0 || slot >= numSources) return "Slot no válido";
    if (! isSupportedFile (file))       return "Formato no soportado";

    SourceSample s;
    if (! decodeAudio (file.createInputStream().release(), s, true))
        return "No se pudo leer el archivo";

    s.name = file.getFileName();
    s.path = file.getFullPathName();
    {
        const juce::ScopedLock sl (sourceLock);
        sources[(size_t) slot] = std::move (s);
    }
    renderNow();
    return {};
}

void SamuraiProcessor::clearSource (int slot)
{
    {
        const juce::ScopedLock sl (sourceLock);
        sources[(size_t) slot].clear();
    }
    renderNow();
}

juce::String SamuraiProcessor::getSourceName (int slot) const
{
    const juce::ScopedLock sl (sourceLock);
    return sources[(size_t) slot].name;
}

bool SamuraiProcessor::isSourceLoaded (int slot) const
{
    const juce::ScopedLock sl (sourceLock);
    return sources[(size_t) slot].isLoaded();
}

//==============================================================================
bool SamuraiProcessor::exportKick (const juce::File& file)
{
    const auto buf = getRenderedCopy();
    if (buf.getNumSamples() == 0) return false;

    const auto ext = file.getFileExtension().toLowerCase();
    std::unique_ptr<juce::AudioFormat> format;
    if (ext == ".flac")                        format = std::make_unique<juce::FlacAudioFormat>();
    else if (ext == ".aif" || ext == ".aiff")  format = std::make_unique<juce::AiffAudioFormat>();
    else                                       format = std::make_unique<juce::WavAudioFormat>();

    file.deleteFile();
    auto os = file.createOutputStream();
    if (os == nullptr) return false;

    std::unique_ptr<juce::AudioFormatWriter> writer (
        format->createWriterFor (os.get(), getRenderSampleRate(), 2u, 24, {}, 0));
    if (writer == nullptr) return false;
    os.release(); // the writer owns the stream now

    return writer->writeFromAudioSampleBuffer (buf, 0, buf.getNumSamples());
}

juce::File SamuraiProcessor::exportTempKick()
{
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("TheSamurai");
    dir.createDirectory();
    auto f = dir.getChildFile ("Samurai Kick " + juce::Time::getCurrentTime().formatted ("%H%M%S") + ".wav");
    return exportKick (f) ? f : juce::File();
}

//==============================================================================
void SamuraiProcessor::setParam (const juce::String& id, float plainValue)
{
    if (auto* p = apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, p->convertTo0to1 (plainValue)));
        p->endChangeGesture();
    }
}

void SamuraiProcessor::explore()
{
    std::vector<int> loaded;
    for (int i = 0; i < numSources; ++i)
        if (isSourceLoaded (i)) loaded.push_back (i);
    if (loaded.empty()) return;

    backState = apvts.copyState();
    auto& rnd = juce::Random::getSystemRandom();
    auto pick = [&] { return loaded[(size_t) rnd.nextInt ((int) loaded.size())]; };

    bool locked[numParts];
    for (int pt = 0; pt < numParts; ++pt)
        locked[pt] = raw (part (pt, "lock")) > 0.5f;

    for (int pt = 0; pt < numParts; ++pt)
    {
        if (locked[pt]) continue;

        const int a = pick();
        setParam (part (pt, "srcA"), (float) a);

        if (loaded.size() > 1 && rnd.nextFloat() < 0.6f)
        {
            int b = pick();
            while (b == a) b = pick();
            setParam (part (pt, "srcB"), (float) (b + 1));
            setParam (part (pt, "blend"), 0.2f + 0.6f * rnd.nextFloat());
        }
        else
        {
            setParam (part (pt, "srcB"), 0.0f);
        }

        setParam (part (pt, "level"), -2.0f + 4.0f * rnd.nextFloat());
    }

    if (! locked[0] && ! locked[1])
        setParam (punchEnd, 12.0f + 50.0f * rnd.nextFloat());

    if (! locked[1] && ! locked[2])
    {
        const float minBody = raw (punchEnd) + raw (xfade) + 20.0f;
        setParam (bodyEnd, juce::jmax (minBody, 90.0f + 220.0f * rnd.nextFloat()));
    }

    renderNow();
    triggerPreview();
}

void SamuraiProcessor::goBack()
{
    if (! backState.isValid()) return;
    apvts.replaceState (backState.createCopy());
    backState = {};
    renderNow();
    triggerPreview();
}

//==============================================================================
std::unique_ptr<juce::XmlElement> SamuraiProcessor::createStateXml()
{
    auto state = apvts.copyState();
    juce::ValueTree srcTree ("SOURCES");

    const juce::ScopedLock sl (sourceLock);
    for (int i = 0; i < numSources; ++i)
    {
        const auto& s = sources[(size_t) i];
        if (! s.isLoaded()) continue;

        juce::ValueTree t ("SRC");
        t.setProperty ("slot", i, nullptr);
        t.setProperty ("name", s.name, nullptr);
        t.setProperty ("path", s.path, nullptr);

        // Embed the audio as FLAC so projects/presets are self-contained.
        juce::MemoryBlock mb;
        {
            juce::FlacAudioFormat flac;
            auto* mos = new juce::MemoryOutputStream (mb, false);
            std::unique_ptr<juce::AudioFormatWriter> w (flac.createWriterFor (mos, s.sampleRate, 2u, 24, {}, 0));
            if (w != nullptr)
            {
                w->writeFromAudioSampleBuffer (s.audio, 0, s.audio.getNumSamples());
            }
            else
            {
                delete mos;
                mb.reset();
                juce::WavAudioFormat wav;
                auto* mos2 = new juce::MemoryOutputStream (mb, false);
                std::unique_ptr<juce::AudioFormatWriter> w2 (wav.createWriterFor (mos2, s.sampleRate, 2u, 32, {}, 0));
                if (w2 != nullptr) w2->writeFromAudioSampleBuffer (s.audio, 0, s.audio.getNumSamples());
                else               delete mos2;
            }
        }
        if (mb.getSize() > 0)
            t.setProperty ("audio", mb.toBase64Encoding(), nullptr);

        srcTree.appendChild (t, nullptr);
    }

    state.appendChild (srcTree, nullptr);
    return state.createXml();
}

void SamuraiProcessor::restoreStateXml (const juce::XmlElement& xml)
{
    auto tree = juce::ValueTree::fromXml (xml);
    if (! tree.isValid() || ! tree.hasType (apvts.state.getType()))
        return;

    auto srcTree = tree.getChildWithName ("SOURCES");
    if (srcTree.isValid())
        tree.removeChild (srcTree, nullptr);

    std::array<SourceSample, numSources> loadedSources;
    if (srcTree.isValid())
    {
        for (auto t : srcTree)
        {
            const int slot = t.getProperty ("slot", -1);
            if (slot < 0 || slot >= numSources) continue;

            auto& s = loadedSources[(size_t) slot];
            bool ok = false;

            const juce::String b64 = t.getProperty ("audio").toString();
            if (b64.isNotEmpty())
            {
                juce::MemoryBlock mb;
                if (mb.fromBase64Encoding (b64))
                    ok = decodeAudio (new juce::MemoryInputStream (mb, true), s, false);
            }

            const juce::File f (t.getProperty ("path").toString());
            if (! ok && f.existsAsFile())
                ok = decodeAudio (f.createInputStream().release(), s, true);

            if (ok)
            {
                s.name = t.getProperty ("name").toString();
                s.path = f.getFullPathName();
            }
        }
    }

    {
        const juce::ScopedLock sl (sourceLock);
        sources = std::move (loadedSources);
    }

    apvts.replaceState (tree);
    backState = {};
    dirty.store (true);
    renderNow();
}

void SamuraiProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = createStateXml())
        copyXmlToBinary (*xml, destData);
}

void SamuraiProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        restoreStateXml (*xml);
}

bool SamuraiProcessor::savePreset (const juce::File& file)
{
    auto xml = createStateXml();
    return xml != nullptr && xml->writeTo (file);
}

bool SamuraiProcessor::loadPreset (const juce::File& file)
{
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr) return false;
    restoreStateXml (*xml);
    return true;
}

//==============================================================================
juce::AudioProcessorEditor* SamuraiProcessor::createEditor()
{
    return new SamuraiEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SamuraiProcessor();
}
