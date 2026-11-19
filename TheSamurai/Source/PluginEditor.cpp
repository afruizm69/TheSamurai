#include "PluginEditor.h"

using namespace ParamIDs;
namespace C = SamuraiColours;

//==============================================================================
SamuraiLookAndFeel::SamuraiLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, C::black);
    setColour (juce::Label::textColourId, C::white);

    setColour (juce::Slider::textBoxTextColourId, C::white);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::rotarySliderFillColourId, C::green);
    setColour (juce::Slider::rotarySliderOutlineColourId, C::track);

    setColour (juce::TextButton::buttonColourId, C::panel);
    setColour (juce::TextButton::buttonOnColourId, C::green);
    setColour (juce::TextButton::textColourOffId, C::white);
    setColour (juce::TextButton::textColourOnId, C::black);

    setColour (juce::ComboBox::backgroundColourId, C::black);
    setColour (juce::ComboBox::textColourId, C::white);
    setColour (juce::ComboBox::outlineColourId, C::panelEdge);
    setColour (juce::ComboBox::arrowColourId, C::green);

    setColour (juce::PopupMenu::backgroundColourId, C::panel);
    setColour (juce::PopupMenu::textColourId, C::white);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, C::green);
    setColour (juce::PopupMenu::highlightedTextColourId, C::black);
}

void SamuraiLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                           float startAngle, float endAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (5.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const juce::PathStrokeType stroke (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    juce::Path bg;
    bg.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour (C::track);
    g.strokePath (bg, stroke);

    juce::Path val;
    val.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, angle, true);
    g.setColour (C::green);
    g.strokePath (val, stroke);

    const float inner = radius * 0.68f;
    g.setColour (C::panel);
    g.fillEllipse (centre.x - inner, centre.y - inner, inner * 2.0f, inner * 2.0f);
    g.setColour (C::panelEdge);
    g.drawEllipse (centre.x - inner, centre.y - inner, inner * 2.0f, inner * 2.0f, 1.0f);

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.5f, -inner, 3.0f, inner * 0.55f, 1.5f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
    g.setColour (C::white);
    g.fillPath (pointer);
}

//==============================================================================
Knob::Knob (APVTS& s, const juce::String& id, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    label.setColour (juce::Label::textColourId, C::grey);
    addAndMakeVisible (label);

    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 15);
    addAndMakeVisible (slider);

    attachment = std::make_unique<APVTS::SliderAttachment> (s, id, slider);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (14));
    slider.setBounds (r);
}

Combo::Combo (APVTS& s, const juce::String& id, const juce::String& text, int labelWidth)
    : labelW (labelWidth)
{
    label.setText (text, juce::dontSendNotification);
    label.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    label.setColour (juce::Label::textColourId, C::grey);
    addAndMakeVisible (label);

    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (id)))
        box.addItemList (choice->choices, 1);
    addAndMakeVisible (box);

    attachment = std::make_unique<APVTS::ComboBoxAttachment> (s, id, box);
}

void Combo::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromLeft (labelW));
    box.setBounds (r);
}

Toggle::Toggle (APVTS& s, const juce::String& id, const juce::String& text)
{
    button.setButtonText (text);
    button.setClickingTogglesState (true);
    addAndMakeVisible (button);
    attachment = std::make_unique<APVTS::ButtonAttachment> (s, id, button);
}

//==============================================================================
SourceSlot::SourceSlot (SamuraiProcessor& p, int slotIndex)
    : proc (p), slot (slotIndex),
      gain   (p.apvts, src (slotIndex, "gain"),   "GAIN"),
      offset (p.apvts, src (slotIndex, "offset"), "OFFSET"),
      invert (p.apvts, src (slotIndex, "invert"), "PHASE")
{
    nameLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
    nameLabel.setMinimumHorizontalScale (0.6f);
    addAndMakeVisible (nameLabel);

    loadButton.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Cargar kick", juce::File(), proc.getSupportedWildcard());
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc)
                              {
                                  const auto f = fc.getResult();
                                  if (f.existsAsFile()) loadFile (f);
                              });
    };
    clearButton.onClick = [this] { proc.clearSource (slot); error = {}; refresh(); };

    addAndMakeVisible (loadButton);
    addAndMakeVisible (clearButton);
    addAndMakeVisible (gain);
    addAndMakeVisible (offset);
    addAndMakeVisible (invert);
    refresh();
}

void SourceSlot::loadFile (const juce::File& f)
{
    error = proc.loadSource (slot, f);
    refresh();
}

void SourceSlot::refresh()
{
    if (error.isNotEmpty())
    {
        nameLabel.setText (error, juce::dontSendNotification);
        nameLabel.setColour (juce::Label::textColourId, juce::Colours::orangered);
    }
    else if (proc.isSourceLoaded (slot))
    {
        nameLabel.setText (proc.getSourceName (slot), juce::dontSendNotification);
        nameLabel.setColour (juce::Label::textColourId, C::white);
    }
    else
    {
        nameLabel.setText ("Suelta WAV / FLAC / AIFF", juce::dontSendNotification);
        nameLabel.setColour (juce::Label::textColourId, C::grey);
    }
    repaint();
}

bool SourceSlot::isInterestedInFileDragging (const juce::StringArray& files)
{
    for (auto& f : files)
        if (proc.isSupportedFile (juce::File (f)))
            return true;
    return false;
}

void SourceSlot::filesDropped (const juce::StringArray& files, int, int)
{
    dragOver = false;
    for (auto& f : files)
    {
        const juce::File file (f);
        if (proc.isSupportedFile (file)) { loadFile (file); break; }
    }
    repaint();
}

void SourceSlot::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (C::panel);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (dragOver ? C::green : (proc.isSourceLoaded (slot) ? C::greenDark : C::panelEdge));
    g.drawRoundedRectangle (r, 6.0f, dragOver ? 2.0f : 1.0f);

    const char* letters[] = { "A", "B", "C", "D" };
    g.setColour (C::green);
    g.setFont (juce::Font (juce::FontOptions (20.0f, juce::Font::bold)));
    g.drawText (letters[slot], 8, 4, 20, 24, juce::Justification::centredLeft);
}

void SourceSlot::resized()
{
    auto r = getLocalBounds().reduced (6);
    auto top = r.removeFromTop (24);
    top.removeFromLeft (22);
    clearButton.setBounds (top.removeFromRight (24));
    top.removeFromRight (4);
    loadButton.setBounds (top.removeFromRight (48));
    top.removeFromRight (4);
    nameLabel.setBounds (top);

    r.removeFromTop (2);
    const int kw = r.getWidth() / 3;
    gain.setBounds (r.removeFromLeft (kw));
    offset.setBounds (r.removeFromLeft (kw));
    invert.setBounds (r.withSizeKeepingCentre (juce::jmin (r.getWidth() - 8, 64), 24));
}

//==============================================================================
WaveformView::WaveformView (APVTS& s)
    : state (s),
      punchParam (s.getParameter (punchEnd)),
      bodyParam  (s.getParameter (bodyEnd))
{
}

float WaveformView::msToX (float ms) const
{
    const float lenMs = state.getRawParameterValue (length)->load();
    return (float) getWidth() * juce::jlimit (0.0f, 1.0f, ms / lenMs);
}

float WaveformView::xToMs (float x) const
{
    const float lenMs = state.getRawParameterValue (length)->load();
    return juce::jlimit (0.0f, 1.0f, x / (float) juce::jmax (1, getWidth())) * lenMs;
}

int WaveformView::hitBoundary (float x) const
{
    const float x1 = msToX (punchParam->convertFrom0to1 (punchParam->getValue()));
    const float x2 = msToX (bodyParam->convertFrom0to1 (bodyParam->getValue()));
    const float d1 = std::abs (x - x1), d2 = std::abs (x - x2);
    if (d1 < 8.0f && d1 <= d2) return 1;
    if (d2 < 8.0f) return 2;
    return 0;
}

void WaveformView::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (C::black);
    g.fillRoundedRectangle (bounds, 6.0f);

    const float w = bounds.getWidth(), h = bounds.getHeight();
    const float x1 = msToX (punchParam->convertFrom0to1 (punchParam->getValue()));
    const float x2 = juce::jmax (x1, msToX (bodyParam->convertFrom0to1 (bodyParam->getValue())));

    // Part regions
    g.setColour (C::green.withAlpha (0.16f));  g.fillRect (0.0f, 0.0f, x1, h);
    g.setColour (C::green.withAlpha (0.07f));  g.fillRect (x1, 0.0f, x2 - x1, h);
    g.setColour (C::white.withAlpha (0.04f));  g.fillRect (x2, 0.0f, w - x2, h);

    // Centre line
    g.setColour (C::track);
    g.drawHorizontalLine ((int) (h * 0.5f), 0.0f, w);

    // Waveform
    const int N = buffer.getNumSamples();
    if (N > 1 && buffer.getNumChannels() >= 2)
    {
        const float* L = buffer.getReadPointer (0);
        const float* R = buffer.getReadPointer (1);
        const float mid = h * 0.5f, scale = h * 0.46f;

        for (int px = 0; px < (int) w; ++px)
        {
            const int s0 = (int) ((juce::int64) px * N / (juce::int64) w);
            const int s1 = juce::jmax (s0 + 1, (int) ((juce::int64) (px + 1) * N / (juce::int64) w));
            float lo = 1.0e9f, hi = -1.0e9f;
            for (int i = s0; i < juce::jmin (s1, N); ++i)
            {
                const float v = 0.5f * (L[i] + R[i]);
                lo = juce::jmin (lo, v);
                hi = juce::jmax (hi, v);
            }
            const bool clip = hi > 1.0f || lo < -1.0f;
            g.setColour ((float) px < x1 ? C::green : ((float) px < x2 ? C::green.withAlpha (0.75f) : C::white.withAlpha (0.8f)));
            if (clip) g.setColour (juce::Colours::orangered);
            g.drawVerticalLine (px, mid - juce::jlimit (-1.0f, 1.0f, hi) * scale, mid - juce::jlimit (-1.0f, 1.0f, lo) * scale + 1.0f);
        }
    }

    // Boundaries
    for (float bx : { x1, x2 })
    {
        g.setColour (C::white);
        g.drawLine (bx, 0.0f, bx, h, 1.5f);
        g.fillRoundedRectangle (bx - 5.0f, 2.0f, 10.0f, 14.0f, 3.0f);
    }

    g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    g.setColour (C::white.withAlpha (0.85f));
    g.drawText ("PUNCH", juce::Rectangle<float> (6.0f, h - 18.0f, juce::jmax (40.0f, x1 - 10.0f), 14.0f), juce::Justification::centredLeft);
    g.drawText ("BODY",  juce::Rectangle<float> (x2 > x1 + 40 ? x1 + 8.0f : x1 + 8.0f, h - 18.0f, 60.0f, 14.0f), juce::Justification::centredLeft);
    g.drawText ("TAIL",  juce::Rectangle<float> (x2 + 8.0f, h - 18.0f, 60.0f, 14.0f), juce::Justification::centredLeft);

    g.setColour (C::panelEdge);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);
}

void WaveformView::mouseMove (const juce::MouseEvent& e)
{
    setMouseCursor (hitBoundary ((float) e.x) != 0 ? juce::MouseCursor::LeftRightResizeCursor
                                                   : juce::MouseCursor::NormalCursor);
}

void WaveformView::mouseDown (const juce::MouseEvent& e)
{
    dragging = hitBoundary ((float) e.x);
    if (dragging == 1) punchParam->beginChangeGesture();
    if (dragging == 2) bodyParam->beginChangeGesture();
}

void WaveformView::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging == 0) return;
    auto* p = dragging == 1 ? punchParam : bodyParam;
    const float ms = xToMs ((float) e.x);
    p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, p->convertTo0to1 (ms)));
    repaint();
}

void WaveformView::mouseUp (const juce::MouseEvent&)
{
    if (dragging == 1) punchParam->endChangeGesture();
    if (dragging == 2) bodyParam->endChangeGesture();
    dragging = 0;
}

//==============================================================================
PartPanel::PartPanel (APVTS& s, int partIndex)
    : index (partIndex),
      srcA  (s, part (partIndex, "srcA"),  "A"),
      srcB  (s, part (partIndex, "srcB"),  "B"),
      blend (s, part (partIndex, "blend"), "BLEND"),
      level (s, part (partIndex, "level"), "LEVEL"),
      hp    (s, part (partIndex, "hp"),    "LOW CUT"),
      lp    (s, part (partIndex, "lp"),    "HIGH CUT"),
      solo  (s, part (partIndex, "solo"),  "SOLO"),
      lock  (s, part (partIndex, "lock"),  "LOCK")
{
    for (auto* c : std::initializer_list<juce::Component*> { &srcA, &srcB, &blend, &level, &hp, &lp, &solo, &lock })
        addAndMakeVisible (c);
}

void PartPanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (C::panel);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (C::panelEdge);
    g.drawRoundedRectangle (r, 6.0f, 1.0f);

    g.setColour (index == 2 ? C::white : C::green);
    g.fillRect (10, 8, 4, 14);
    g.setFont (juce::Font (juce::FontOptions (14.0f, juce::Font::bold)));
    g.drawText (partTitle (index), 20, 6, 120, 18, juce::Justification::centredLeft);
}

void PartPanel::resized()
{
    auto r = getLocalBounds().reduced (8);
    r.removeFromTop (22);

    auto combos = r.removeFromTop (24);
    srcA.setBounds (combos.removeFromLeft (combos.getWidth() / 2).reduced (2, 0));
    srcB.setBounds (combos.reduced (2, 0));
    r.removeFromTop (6);

    auto toggles = r.removeFromBottom (26);
    solo.setBounds (toggles.removeFromLeft (toggles.getWidth() / 2).reduced (4, 0));
    lock.setBounds (toggles.reduced (4, 0));
    r.removeFromBottom (4);

    const int kw = r.getWidth() / 4;
    blend.setBounds (r.removeFromLeft (kw));
    level.setBounds (r.removeFromLeft (kw));
    hp.setBounds (r.removeFromLeft (kw));
    lp.setBounds (r);
}

//==============================================================================
void DragHandle::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (C::green);
    g.drawRoundedRectangle (r, 4.0f, 1.5f);
    g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    g.drawText ("DRAG TO DAW", getLocalBounds(), juce::Justification::centred);
}

void DragHandle::mouseDrag (const juce::MouseEvent& e)
{
    if (started || e.getDistanceFromDragStart() < 5 || ! makeFile)
        return;

    started = true;
    const auto f = makeFile();
    if (f.existsAsFile())
        juce::DragAndDropContainer::performExternalDragDropOfFiles (juce::StringArray (f.getFullPathName()), false, this);
}

//==============================================================================
SamuraiEditor::SamuraiEditor (SamuraiProcessor& p)
    : AudioProcessorEditor (&p), proc (p), wave (p.apvts)
{
    setLookAndFeel (&lnf);

    for (auto* b : { &playButton, &exploreButton, &backButton, &exportButton, &saveButton, &openButton })
        addAndMakeVisible (b);
    exploreButton.setColour (juce::TextButton::buttonColourId, C::greenDark);

    playButton.onClick    = [this] { proc.triggerPreview(); };
    exploreButton.onClick = [this] { proc.explore(); };
    backButton.onClick    = [this] { proc.goBack(); };

    exportButton.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Exportar kick (.wav / .flac / .aiff)",
                                                       juce::File::getSpecialLocation (juce::File::userDesktopDirectory).getChildFile ("Samurai Kick.wav"),
                                                       "*.wav;*.flac;*.aif;*.aiff");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                              [this] (const juce::FileChooser& fc)
                              {
                                  auto f = fc.getResult();
                                  if (f == juce::File()) return;
                                  if (f.getFileExtension().isEmpty()) f = f.withFileExtension (".wav");
                                  proc.exportKick (f);
                              });
    };

    saveButton.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Guardar diseño",
                                                       juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Mi kick.samurai"),
                                                       "*.samurai");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                              [this] (const juce::FileChooser& fc)
                              {
                                  auto f = fc.getResult();
                                  if (f == juce::File()) return;
                                  proc.savePreset (f.withFileExtension (".samurai"));
                              });
    };

    openButton.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Abrir diseño",
                                                       juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
                                                       "*.samurai");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc)
                              {
                                  const auto f = fc.getResult();
                                  if (f.existsAsFile()) proc.loadPreset (f);
                              });
    };

    dragHandle.makeFile = [this] { return proc.exportTempKick(); };
    addAndMakeVisible (dragHandle);

    for (int i = 0; i < numSources; ++i)
        addAndMakeVisible (slots.add (new SourceSlot (proc, i)));

    addAndMakeVisible (wave);

    for (int i = 0; i < numParts; ++i)
        addAndMakeVisible (parts.add (new PartPanel (proc.apvts, i)));

    auto& s = proc.apvts;
    for (int i = 0; i < numStages; ++i)
        addAndMakeVisible (stageCombos.add (new Combo (s, stage (i), juce::String (i + 1), 12)));

    addAndMakeVisible (sculptKnobs.add (new Knob (s, impact,    "IMPACT")));
    addAndMakeVisible (sculptKnobs.add (new Knob (s, drive,     "DRIVE")));
    addAndMakeVisible (sculptKnobs.add (new Knob (s, tone,      "TONE")));
    addAndMakeVisible (sculptKnobs.add (new Knob (s, filterCut, "FILTER")));
    addAndMakeVisible (sculptKnobs.add (new Knob (s, filterRes, "RESO")));

    addAndMakeVisible (eqKnobs.add (new Knob (s, eqLow,   "LOW 60")));
    addAndMakeVisible (eqKnobs.add (new Knob (s, eqMid1F, "MID 1 FREQ")));
    addAndMakeVisible (eqKnobs.add (new Knob (s, eqMid1G, "MID 1 GAIN")));
    addAndMakeVisible (eqKnobs.add (new Knob (s, eqHigh,  "HIGH 8K")));
    addAndMakeVisible (eqKnobs.add (new Knob (s, eqMid2F, "MID 2 FREQ")));
    addAndMakeVisible (eqKnobs.add (new Knob (s, eqMid2G, "MID 2 GAIN")));

    addAndMakeVisible (outKnobs.add (new Knob (s, length,   "LENGTH")));
    addAndMakeVisible (outKnobs.add (new Knob (s, fade,     "FADE")));
    addAndMakeVisible (outKnobs.add (new Knob (s, xfade,    "X-FADE")));
    addAndMakeVisible (outKnobs.add (new Knob (s, tune,     "TUNE")));
    addAndMakeVisible (outKnobs.add (new Knob (s, monoFreq, "MONO <")));
    addAndMakeVisible (outKnobs.add (new Knob (s, outGain,  "OUTPUT")));
    addAndMakeVisible (outKnobs.add (new Knob (s, velSens,  "VELOCITY")));

    addAndMakeVisible (outToggles.add (new Toggle (s, monoOn,   "MONO BASS")));
    addAndMakeVisible (outToggles.add (new Toggle (s, keyTrack, "KEY TRACK")));

    setSize (1080, 790);
    startTimerHz (20);
}

SamuraiEditor::~SamuraiEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void SamuraiEditor::timerCallback()
{
    const int v = proc.getRenderVersion();
    if (v != lastVersion)
    {
        lastVersion = v;
        wave.setBuffer (proc.getRenderedCopy());
        for (auto* s : slots) s->refresh();
    }
    backButton.setEnabled (proc.canGoBack());
}

void SamuraiEditor::drawPanel (juce::Graphics& g, juce::Rectangle<int> area, const juce::String& title)
{
    auto r = area.toFloat().reduced (0.5f);
    g.setColour (C::panel);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (C::panelEdge);
    g.drawRoundedRectangle (r, 6.0f, 1.0f);

    g.setColour (C::green);
    g.fillRect (area.getX() + 10, area.getY() + 8, 4, 14);
    g.setColour (C::white);
    g.setFont (juce::Font (juce::FontOptions (14.0f, juce::Font::bold)));
    g.drawText (title, area.getX() + 20, area.getY() + 6, 200, 18, juce::Justification::centredLeft);
}

void SamuraiEditor::paint (juce::Graphics& g)
{
    g.fillAll (C::black);

    // Header
    g.setColour (C::green);
    g.fillRect (0, 0, getWidth(), 3);

    g.setColour (C::white);
    g.setFont (juce::Font (juce::FontOptions (28.0f, juce::Font::bold)));
    g.drawText ("THE", 16, 10, 60, 36, juce::Justification::centredLeft);
    g.setColour (C::green);
    g.drawText ("SAMURAI", 72, 10, 200, 36, juce::Justification::centredLeft);

    g.setColour (C::grey);
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.drawText ("KICK DESIGNER  /  WAV - FLAC - AIFF", 236, 22, 260, 16, juce::Justification::centredLeft);

    drawPanel (g, sculptArea, "SCULPT");
    drawPanel (g, eqArea, "EQ");
    drawPanel (g, outArea, "OUTPUT");

    g.setColour (C::grey);
    g.setFont (juce::Font (juce::FontOptions (10.0f)));
    g.drawText ("Orden de la cadena", sculptArea.getX() + 80, sculptArea.getY() + 6, 200, 18, juce::Justification::centredLeft);
}

void SamuraiEditor::resized()
{
    auto r = getLocalBounds().reduced (10);

    // Header buttons
    auto header = r.removeFromTop (44);
    header.removeFromTop (6);
    auto addBtn = [&header] (juce::Component& c, int w) { c.setBounds (header.removeFromRight (w).reduced (3, 2)); };
    addBtn (dragHandle, 112);
    addBtn (openButton, 64);
    addBtn (saveButton, 64);
    addBtn (exportButton, 76);
    addBtn (backButton, 64);
    addBtn (exploreButton, 88);
    addBtn (playButton, 64);

    r.removeFromTop (6);

    // Sources
    auto srcRow = r.removeFromTop (110);
    const int sw = (srcRow.getWidth() - 3 * 8) / 4;
    for (int i = 0; i < slots.size(); ++i)
    {
        slots[i]->setBounds (srcRow.removeFromLeft (sw));
        srcRow.removeFromLeft (8);
    }

    r.removeFromTop (8);
    wave.setBounds (r.removeFromTop (150));
    r.removeFromTop (8);

    // Parts
    auto partRow = r.removeFromTop (196);
    const int pw = (partRow.getWidth() - 2 * 8) / 3;
    for (int i = 0; i < parts.size(); ++i)
    {
        parts[i]->setBounds (partRow.removeFromLeft (pw));
        partRow.removeFromLeft (8);
    }

    r.removeFromTop (8);

    // Bottom panels
    auto bottom = r;
    const int total = bottom.getWidth() - 16;
    sculptArea = bottom.removeFromLeft ((int) (total * 0.38f));
    bottom.removeFromLeft (8);
    eqArea = bottom.removeFromLeft ((int) (total * 0.27f));
    bottom.removeFromLeft (8);
    outArea = bottom;

    {
        auto a = sculptArea.reduced (8);
        a.removeFromTop (24);
        auto combos = a.removeFromTop (24);
        const int cw = combos.getWidth() / numStages;
        for (auto* c : stageCombos)
            c->setBounds (combos.removeFromLeft (cw).reduced (2, 0));
        a.removeFromTop (10);
        auto knobs = a.removeFromTop (juce::jmin (a.getHeight(), 104));
        const int kw = knobs.getWidth() / sculptKnobs.size();
        for (auto* k : sculptKnobs)
            k->setBounds (knobs.removeFromLeft (kw));
    }

    {
        auto a = eqArea.reduced (8);
        a.removeFromTop (22);
        const int rowH = a.getHeight() / 2;
        for (int row = 0; row < 2; ++row)
        {
            auto line = a.removeFromTop (rowH);
            const int kw = line.getWidth() / 3;
            for (int i = 0; i < 3; ++i)
                eqKnobs[row * 3 + i]->setBounds (line.removeFromLeft (kw));
        }
    }

    {
        auto a = outArea.reduced (8);
        a.removeFromTop (22);
        const int rowH = a.getHeight() / 2;
        auto row1 = a.removeFromTop (rowH);
        const int kw = row1.getWidth() / 4;
        for (int i = 0; i < 4; ++i)
            outKnobs[i]->setBounds (row1.removeFromLeft (kw));

        auto row2 = a;
        for (int i = 4; i < 7; ++i)
            outKnobs[i]->setBounds (row2.removeFromLeft (kw));
        auto toggles = row2.reduced (4, 14);
        outToggles[0]->setBounds (toggles.removeFromTop (toggles.getHeight() / 2).reduced (0, 3));
        outToggles[1]->setBounds (toggles.reduced (0, 3));
    }
}
