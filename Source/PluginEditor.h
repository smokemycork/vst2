#pragma once
#include "PluginProcessor.h"

namespace TBColours
{
const juce::Colour bg      { 0xff0d0e12 };
const juce::Colour panel   { 0xff171920 };
const juce::Colour track   { 0xff2a2d38 };
const juce::Colour accent  { 0xffff6a2b };
const juce::Colour accent2 { 0xffffb347 };
const juce::Colour text    { 0xffe8e8ee };
const juce::Colour dim     { 0xff8a8d99 };
}

struct TensionLookAndFeel : public juce::LookAndFeel_V4
{
    TensionLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
};

struct Knob
{
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    void init (juce::Component& parent, juce::AudioProcessorValueTreeState& state,
               const char* paramID, const juce::String& text, bool big = false);
    void setBounds (juce::Rectangle<int> area, int labelHeight);
};

class TensionEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TensionEditor (TensionProcessor&);
    ~TensionEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    juce::Rectangle<int> barBounds() const { return { 30, 425, getWidth() - 60, 12 }; }

    TensionProcessor& proc;
    TensionLookAndFeel laf;

    Knob build, curve, filter, noise, stutter, reverb, drive, width, impact, mix, output;

    juce::ComboBox autoBox, rateBox, presetBox;
    juce::Label autoLabel, rateLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> autoAtt, rateAtt;

    juce::TextButton dropButton { "DROP  (hold)" };
    bool dropDown = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TensionEditor)
};
