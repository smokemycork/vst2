#include "PluginEditor.h"

//==============================================================================
TensionLookAndFeel::TensionLookAndFeel()
{
    using namespace TBColours;
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, dim);
    setColour (juce::ComboBox::backgroundColourId, panel);
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::ComboBox::outlineColourId, track);
    setColour (juce::ComboBox::arrowColourId, accent);
    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::textColourId, text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, accent);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
    setColour (juce::TextButton::buttonColourId, panel);
    setColour (juce::TextButton::buttonOnColourId, accent);
    setColour (juce::TextButton::textColourOffId, text);
    setColour (juce::TextButton::textColourOnId, juce::Colours::black);
}

void TensionLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                           float startAngle, float endAngle, juce::Slider& s)
{
    using namespace TBColours;

    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float diameter = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto centre = bounds.getCentre();
    const float radius = diameter * 0.5f;
    const bool big = s.getName() == "big";
    const float arcW = big ? 9.0f : 4.0f;
    const float arcR = radius - arcW * 0.5f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    juce::Path trackPath;
    trackPath.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (track);
    g.strokePath (trackPath, juce::PathStrokeType (arcW, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

    if (pos > 0.001f)
    {
        juce::Path valuePath;
        valuePath.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, angle, true);
        g.setGradientFill (juce::ColourGradient (accent2, bounds.getX(), bounds.getBottom(),
                                                 accent, bounds.getRight(), bounds.getY(), false));
        g.strokePath (valuePath, juce::PathStrokeType (arcW, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
    }

    const float bodyR = arcR - arcW - 3.0f;
    g.setColour (juce::Colour (0xff1f222b));
    g.fillEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    g.setColour (juce::Colour (0xff343846));
    g.drawEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.2f);

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.8f, -bodyR + 4.0f, 3.6f, bodyR * 0.45f, 1.5f);
    g.setColour (accent);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
}

//==============================================================================
void Knob::init (juce::Component& parent, juce::AudioProcessorValueTreeState& state,
                 const char* paramID, const juce::String& text, bool big)
{
    slider.setName (big ? "big" : "small");
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, big ? 90 : 70, big ? 22 : 16);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                juce::MathConstants<float>::pi * 2.8f, true);
    slider.setPopupDisplayEnabled (false, false, nullptr);
    parent.addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (juce::FontOptions (big ? 15.0f : 12.0f, juce::Font::bold)));
    label.setColour (juce::Label::textColourId, big ? TBColours::text : TBColours::dim);
    parent.addAndMakeVisible (label);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, paramID, slider);
}

void Knob::setBounds (juce::Rectangle<int> area, int labelHeight)
{
    label.setBounds (area.removeFromTop (labelHeight));
    slider.setBounds (area);
}

//==============================================================================
TensionEditor::TensionEditor (TensionProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    setSize (800, 470);

    auto& s = proc.apvts;
    build.init   (*this, s, ParamID::build,   "BUILD", true);
    curve.init   (*this, s, ParamID::curve,   "CURVE");
    filter.init  (*this, s, ParamID::filter,  "FILTER");
    noise.init   (*this, s, ParamID::noise,   "NOISE");
    stutter.init (*this, s, ParamID::stutter, "STUTTER");
    reverb.init  (*this, s, ParamID::reverb,  "REVERB");
    drive.init   (*this, s, ParamID::drive,   "DRIVE");
    width.init   (*this, s, ParamID::width,   "WIDTH");
    impact.init  (*this, s, ParamID::impact,  "IMPACT");
    mix.init     (*this, s, ParamID::mix,     "MIX");
    output.init  (*this, s, ParamID::output,  "OUTPUT");

    // combos
    autoBox.addItemList ({ "Off (use Build)", "4 bars", "8 bars", "16 bars", "32 bars" }, 1);
    rateBox.addItemList ({ "Auto (ramps up)", "1/4", "1/8", "1/16", "1/32" }, 1);
    addAndMakeVisible (autoBox);
    addAndMakeVisible (rateBox);
    autoAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, ParamID::autoLen, autoBox);
    rateAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, ParamID::rate, rateBox);

    for (auto* l : { &autoLabel, &rateLabel })
    {
        l->setJustificationType (juce::Justification::centredLeft);
        l->setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
        l->setColour (juce::Label::textColourId, TBColours::dim);
        addAndMakeVisible (*l);
    }
    autoLabel.setText ("AUTO BUILD LENGTH", juce::dontSendNotification);
    rateLabel.setText ("STUTTER RATE", juce::dontSendNotification);

    // presets
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    // momentary drop button: hold = drop engaged
    dropButton.setClickingTogglesState (false);
    dropButton.onStateChange = [this]
    {
        const bool down = dropButton.isDown();
        if (down == dropDown)
            return;
        dropDown = down;
        if (auto* prm = proc.apvts.getParameter (ParamID::drop))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (down ? 1.0f : 0.0f);
            prm->endChangeGesture();
        }
    };
    addAndMakeVisible (dropButton);

    startTimerHz (30);
}

TensionEditor::~TensionEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void TensionEditor::timerCallback()
{
    if (auto* prm = proc.apvts.getParameter (ParamID::drop))
        dropButton.setToggleState (prm->getValue() > 0.5f, juce::dontSendNotification);
    repaint (barBounds().expanded (2));
}

void TensionEditor::paint (juce::Graphics& g)
{
    using namespace TBColours;

    g.setGradientFill (juce::ColourGradient (bg, 0.0f, 0.0f, juce::Colour (0xff15171e),
                                             0.0f, (float) getHeight(), false));
    g.fillAll();

    // header
    g.setColour (text);
    g.setFont (juce::Font (juce::FontOptions (26.0f, juce::Font::bold)));
    g.drawText ("TENSION BUILDER", 30, 14, 400, 34, juce::Justification::centredLeft);
    g.setColour (accent);
    g.fillRoundedRectangle (30.0f, 50.0f, 56.0f, 3.0f, 1.5f);
    g.setColour (dim);
    g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::plain)));
    g.drawText ("tech house buildup machine", 94, 42, 300, 18, juce::Justification::centredLeft);

    // panels
    g.setColour (panel);
    g.fillRoundedRectangle (20.0f, 78.0f, 260.0f, 330.0f, 14.0f);
    g.fillRoundedRectangle (292.0f, 78.0f, 488.0f, 330.0f, 14.0f);

    // build progress bar
    auto bar = barBounds().toFloat();
    g.setColour (track);
    g.fillRoundedRectangle (bar, 6.0f);
    const float v = juce::jlimit (0.0f, 1.0f, proc.visBuild.load());
    if (v > 0.002f)
    {
        auto fill = bar.withWidth (juce::jmax (12.0f, bar.getWidth() * v));
        g.setGradientFill (juce::ColourGradient (accent2, fill.getX(), 0.0f, accent, bar.getRight(), 0.0f, false));
        g.fillRoundedRectangle (fill, 6.0f);
    }
    g.setColour (dim);
    g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::plain)));
    g.drawText ("TENSION", 30, 440, 100, 16, juce::Justification::centredLeft);
}

void TensionEditor::resized()
{
    presetBox.setBounds (getWidth() - 230, 20, 200, 28);

    // left panel
    build.setBounds ({ 30, 88, 240, 262 }, 22);
    dropButton.setBounds (35, 356, 230, 40);

    // right panel: 5 x 2 knob grid
    const int gx = 300, gy = 90, cw = 94, rh = 128;
    Knob* grid[2][5] = { { &curve, &filter, &noise, &stutter, &reverb },
                         { &drive, &width, &impact, &mix, &output } };
    for (int r = 0; r < 2; ++r)
        for (int c = 0; c < 5; ++c)
            grid[r][c]->setBounds ({ gx + c * cw, gy + r * rh, cw, rh - 6 }, 16);

    // combos
    autoLabel.setBounds (310, 352, 200, 16);
    autoBox.setBounds (310, 370, 220, 28);
    rateLabel.setBounds (550, 352, 200, 16);
    rateBox.setBounds (550, 370, 220, 28);
}
