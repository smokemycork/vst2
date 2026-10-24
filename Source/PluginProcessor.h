#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <vector>
#include "Dsp.h"

namespace ParamID
{
inline constexpr const char* build   = "build";
inline constexpr const char* curve   = "curve";
inline constexpr const char* autoLen = "autoLen";
inline constexpr const char* drop    = "drop";
inline constexpr const char* filter  = "filter";
inline constexpr const char* noise   = "noise";
inline constexpr const char* stutter = "stutter";
inline constexpr const char* rate    = "rate";
inline constexpr const char* reverb  = "reverb";
inline constexpr const char* drive   = "drive";
inline constexpr const char* width   = "width";
inline constexpr const char* impact  = "impact";
inline constexpr const char* mix     = "mix";
inline constexpr const char* output  = "output";
}

class TensionProcessor : public juce::AudioProcessor
{
public:
    TensionProcessor();
    ~TensionProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Tension Builder"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // 0..1 effective build amount, read by the editor for the progress bar
    std::atomic<float> visBuild { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    std::atomic<float>* pBuild   = nullptr;
    std::atomic<float>* pCurve   = nullptr;
    std::atomic<float>* pAuto    = nullptr;
    std::atomic<float>* pDrop    = nullptr;
    std::atomic<float>* pFilter  = nullptr;
    std::atomic<float>* pNoise   = nullptr;
    std::atomic<float>* pStutter = nullptr;
    std::atomic<float>* pRate    = nullptr;
    std::atomic<float>* pReverb  = nullptr;
    std::atomic<float>* pDrive   = nullptr;
    std::atomic<float>* pWidth   = nullptr;
    std::atomic<float>* pImpact  = nullptr;
    std::atomic<float>* pMix     = nullptr;
    std::atomic<float>* pOutput  = nullptr;

    double sampleRate = 44100.0;
    float  buildCoef  = 0.01f;
    float  gateCoef   = 0.05f;

    // state
    float  buildSm = 0.f, gateSm = 1.f;
    float  lastKnob = 0.f;
    bool   lastDrop = false;
    double lastAutoPos = 0.0;
    double internalPpq = 0.0;
    uint32_t seedL = 0x1234567u, seedR = 0x9e3779b9u;

    bool   impactActive = false;
    double impactT = 0.0, impactPhase = 0.0;

    tb::SVFState hpS[2], noiseS[2];
    juce::Reverb reverb;

    std::vector<float> procL, procR, revL, revR, bTrack;
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TensionProcessor)
};
