#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
struct Preset
{
    const char* name;
    float curve, rate, filter, noise, stutter, reverb, drive, width, impact, mix;
};

// rate: 0 = Auto, 1 = 1/4, 2 = 1/8, 3 = 1/16, 4 = 1/32
const Preset kPresets[] = {
    { "Classic Riser",       0.40f, 0, 0.80f, 0.70f, 0.35f, 0.50f, 0.30f, 0.50f, 0.60f, 1.0f },
    { "Snare Roll Tension",  0.50f, 0, 0.60f, 0.35f, 0.90f, 0.30f, 0.50f, 0.30f, 0.50f, 1.0f },
    { "Wash Out",            0.30f, 0, 0.90f, 0.50f, 0.10f, 1.00f, 0.15f, 1.00f, 0.70f, 1.0f },
    { "Subtle Lift",         0.30f, 0, 0.45f, 0.20f, 0.20f, 0.25f, 0.10f, 0.30f, 0.30f, 1.0f },
    { "Peak Hour Mayhem",    0.70f, 0, 1.00f, 0.90f, 1.00f, 0.60f, 0.80f, 0.70f, 1.00f, 1.0f },
};
constexpr int kNumPresets = (int) (sizeof (kPresets) / sizeof (kPresets[0]));

constexpr int   kAutoBars[5] = { 0, 4, 8, 16, 32 };
constexpr float kLogFilterRange = 6.10924758f; // ln (9000 / 20)
constexpr float kLogNoiseRange  = 4.02535169f; // ln (14000 / 250)
constexpr float kTwoPi = 6.28318530717958647692f;
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout TensionProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto pct = [] (float v, int) { return String (roundToInt (v * 100.0f)) + " %"; };
    auto pctParse = [] (const String& t) { return t.getFloatValue() / 100.0f; };

    auto addPct = [&] (const char* id, const char* name, float def)
    {
        p.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { id, 1 }, name, NormalisableRange<float> (0.0f, 1.0f, 0.001f), def,
            AudioParameterFloatAttributes().withStringFromValueFunction (pct)
                                           .withValueFromStringFunction (pctParse)));
    };

    addPct (ParamID::build,   "Build",   0.0f);
    addPct (ParamID::curve,   "Curve",   0.4f);
    p.push_back (std::make_unique<AudioParameterChoice> (
        ParameterID { ParamID::autoLen, 1 }, "Auto Length",
        StringArray { "Off (use Build)", "4 bars", "8 bars", "16 bars", "32 bars" }, 0));
    p.push_back (std::make_unique<AudioParameterBool> (
        ParameterID { ParamID::drop, 1 }, "Drop", false));
    addPct (ParamID::filter,  "Filter",  0.8f);
    addPct (ParamID::noise,   "Noise",   0.7f);
    addPct (ParamID::stutter, "Stutter", 0.4f);
    p.push_back (std::make_unique<AudioParameterChoice> (
        ParameterID { ParamID::rate, 1 }, "Stutter Rate",
        StringArray { "Auto (ramps up)", "1/4", "1/8", "1/16", "1/32" }, 0));
    addPct (ParamID::reverb,  "Reverb",  0.5f);
    addPct (ParamID::drive,   "Drive",   0.3f);
    addPct (ParamID::width,   "Width",   0.5f);
    addPct (ParamID::impact,  "Impact",  0.6f);
    addPct (ParamID::mix,     "Mix",     1.0f);
    p.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::output, 1 }, "Output", NormalisableRange<float> (-24.0f, 6.0f, 0.1f), 0.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction (
            [] (float v, int) { return String (v, 1) + " dB"; })));

    return { p.begin(), p.end() };
}

//==============================================================================
TensionProcessor::TensionProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    pBuild   = apvts.getRawParameterValue (ParamID::build);
    pCurve   = apvts.getRawParameterValue (ParamID::curve);
    pAuto    = apvts.getRawParameterValue (ParamID::autoLen);
    pDrop    = apvts.getRawParameterValue (ParamID::drop);
    pFilter  = apvts.getRawParameterValue (ParamID::filter);
    pNoise   = apvts.getRawParameterValue (ParamID::noise);
    pStutter = apvts.getRawParameterValue (ParamID::stutter);
    pRate    = apvts.getRawParameterValue (ParamID::rate);
    pReverb  = apvts.getRawParameterValue (ParamID::reverb);
    pDrive   = apvts.getRawParameterValue (ParamID::drive);
    pWidth   = apvts.getRawParameterValue (ParamID::width);
    pImpact  = apvts.getRawParameterValue (ParamID::impact);
    pMix     = apvts.getRawParameterValue (ParamID::mix);
    pOutput  = apvts.getRawParameterValue (ParamID::output);
}

bool TensionProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo();
}

void TensionProcessor::prepareToPlay (double sr, int samplesPerBlock)
{
    sampleRate = sr;
    buildCoef = 1.0f - std::exp (-1.0f / (0.005f * (float) sr));   // 5 ms
    gateCoef  = 1.0f - std::exp (-1.0f / (0.0015f * (float) sr));  // 1.5 ms

    const auto n = (size_t) juce::jmax (samplesPerBlock, 1);
    procL.assign (n, 0.f); procR.assign (n, 0.f);
    revL.assign (n, 0.f);  revR.assign (n, 0.f);
    bTrack.assign (n, 0.f);

    reverb.setSampleRate (sr);
    reverb.reset();
    for (auto& s : hpS)    s.reset();
    for (auto& s : noiseS) s.reset();

    buildSm = 0.f;
    gateSm = 1.f;
    lastKnob = pBuild->load();
    lastDrop = pDrop->load() > 0.5f;
    lastAutoPos = 0.0;
    internalPpq = 0.0;
    impactActive = false;
}

//==============================================================================
void TensionProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || n <= 0)
        return;

    if ((size_t) n > procL.size())
    {
        const auto sz = (size_t) n;
        procL.resize (sz); procR.resize (sz);
        revL.resize (sz);  revR.resize (sz);
        bTrack.resize (sz);
    }

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);
    const float sr = (float) sampleRate;

    // ---- parameters ------------------------------------------------------
    const float knob       = pBuild->load();
    const float curveExp   = 0.5f + 2.5f * pCurve->load();
    const int   autoBars   = kAutoBars[juce::jlimit (0, 4, (int) pAuto->load())];
    const bool  dropNow    = pDrop->load() > 0.5f;
    const float filterAmt  = pFilter->load();
    const float noiseAmt   = pNoise->load();
    const float stutterAmt = pStutter->load();
    const int   rateIdx    = juce::jlimit (0, 4, (int) pRate->load());
    const float reverbAmt  = pReverb->load();
    const float driveAmt   = pDrive->load();
    const float widthAmt   = pWidth->load();
    const float impactAmt  = pImpact->load();
    const float mix        = pMix->load();
    const float outGain    = juce::Decibels::decibelsToGain (pOutput->load());

    // ---- host timing -----------------------------------------------------
    double bpm = 125.0, ppq = 0.0;
    bool playing = false, havePpq = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            playing = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition()) { ppq = *q; havePpq = true; }
        }
    if (! (bpm >= 20.0 && bpm <= 400.0)) bpm = 125.0;

    const bool hostClock  = playing && havePpq;
    if (! hostClock) ppq = internalPpq;
    const double ppqPerSample = bpm / (60.0 * sampleRate);
    const bool autoActive = autoBars > 0 && hostClock;

    // sample index at which an impact should start (-1 = none)
    int impactAt = -1;
    if (dropNow && ! lastDrop) impactAt = 0;
    lastDrop = dropNow;

    if (! autoActive)
    {
        // Build knob (or its automation) slammed from high to ~0 = the drop
        if (lastKnob > 0.5f && knob < 0.05f)
        {
            if (impactAt < 0) impactAt = 0;
            buildSm = 0.f;
        }
    }
    lastKnob = knob;

    const float noiseFcScale = 250.0f;

    // ---- pass 1: filter / gate / drive / noise / reverb send -------------
    tb::SVFCoeffs hpC, nC;
    for (int i = 0; i < n; ++i)
    {
        const double ppqNow = ppq + (double) i * ppqPerSample;

        float target = knob;
        if (autoActive)
        {
            const double span = (double) autoBars * 4.0;
            double pos = std::fmod (ppqNow, span) / span;
            if (pos < 0.0) pos += 1.0;
            if (pos < lastAutoPos && lastAutoPos > 0.9)   // wrapped = the drop
            {
                if (impactAt < 0) impactAt = i;
                buildSm = 0.f;
            }
            lastAutoPos = pos;
            target = (float) pos;
        }

        if (dropNow) buildSm = 0.f;
        else         buildSm += buildCoef * (target - buildSm);

        const float bb = buildSm;
        const float sb = std::pow (bb, curveExp);
        bTrack[(size_t) i] = sb;

        // rising resonant high-pass
        hpC.set (20.0f * std::exp (kLogFilterRange * sb * filterAmt),
                 0.707f + 2.3f * sb * filterAmt, sr);

        // rhythmic stutter gate, tempo-synced, getting faster as build rises
        int div;
        if (rateIdx == 0) div = bb < 0.30f ? 1 : bb < 0.55f ? 2 : bb < 0.80f ? 4 : 8;
        else              div = 1 << (rateIdx - 1);
        double ph = std::fmod (ppqNow * (double) div, 1.0);
        if (ph < 0.0) ph += 1.0;
        const float depth = stutterAmt * tb::smoothstep (0.10f, 0.85f, bb);
        const float duty  = 0.62f - 0.27f * bb;
        const float gTarget = ph < (double) duty ? 1.0f : 1.0f - depth;
        gateSm += gateCoef * (gTarget - gateSm);

        // rising band-passed noise
        nC.set (noiseFcScale * std::exp (kLogNoiseRange * sb), 1.2f, sr);
        const float nLevel = noiseAmt * 0.9f * sb * sb;
        float tmp, bpL, bpR;
        noiseS[0].process (tb::nextNoise (seedL), nC, tmp, bpL);
        noiseS[1].process (tb::nextNoise (seedR), nC, tmp, bpR);
        const float noiseL = bpL * nC.k * nLevel;
        const float noiseR = bpR * nC.k * nLevel;

        // signal path
        float hpL, hpR, dummy;
        hpS[0].process (L[i], hpC, hpL, dummy);
        hpS[1].process (R[i], hpC, hpR, dummy);

        float gl = hpL * gateSm;
        float gr = hpR * gateSm;

        const float satMix = driveAmt * sb;
        if (satMix > 0.0005f)
        {
            const float g = 1.0f + 2.5f * satMix;
            const float inv = 1.0f / std::sqrt (g);
            gl += satMix * (std::tanh (g * gl) * inv - gl);
            gr += satMix * (std::tanh (g * gr) * inv - gr);
        }

        procL[(size_t) i] = gl + noiseL;
        procR[(size_t) i] = gr + noiseR;

        const float send = reverbAmt * sb * 0.8f;
        revL[(size_t) i] = hpL * send;
        revR[(size_t) i] = hpR * send;
    }

    // ---- reverb wash (tail keeps ringing into the drop) -------------------
    juce::Reverb::Parameters rp;
    rp.roomSize   = 0.55f + 0.40f * buildSm;
    rp.damping    = 0.55f - 0.35f * buildSm;
    rp.wetLevel   = 0.33f;
    rp.dryLevel   = 0.0f;
    rp.width      = 1.0f;
    rp.freezeMode = 0.0f;
    reverb.setParameters (rp);
    reverb.processStereo (revL.data(), revR.data(), n);

    // ---- pass 2: width, mix, impact, output --------------------------------
    for (int i = 0; i < n; ++i)
    {
        if (i == impactAt)
        {
            impactActive = true;
            impactT = 0.0;
            impactPhase = 0.0;
        }

        float wl = procL[(size_t) i] + revL[(size_t) i];
        float wr = procR[(size_t) i] + revR[(size_t) i];

        const float m = 0.5f * (wl + wr);
        const float s = 0.5f * (wl - wr) * (1.0f + widthAmt * 0.8f * bTrack[(size_t) i]);
        wl = m + s;
        wr = m - s;

        float ol = L[i] * (1.0f - mix) + wl * mix;
        float orr = R[i] * (1.0f - mix) + wr * mix;

        if (impactActive)
        {
            const double freq = 42.0 + 110.0 * std::exp (-impactT * 14.0);
            impactPhase += (double) kTwoPi * freq / sampleRate;
            if (impactPhase > (double) kTwoPi) impactPhase -= (double) kTwoPi;
            const float amp = (float) std::exp (-impactT * 3.2);
            const float click = tb::nextNoise (seedL) * (float) std::exp (-impactT * 60.0) * 0.3f;
            const float v = (std::sin ((float) impactPhase) * amp * 0.9f + click) * impactAmt;
            ol += v;
            orr += v;
            impactT += 1.0 / sampleRate;
            if (impactT > 1.5) impactActive = false;
        }

        L[i] = juce::jlimit (-4.0f, 4.0f, ol * outGain);
        R[i] = juce::jlimit (-4.0f, 4.0f, orr * outGain);
    }

    visBuild.store (buildSm);
    internalPpq = ppq + (double) n * ppqPerSample;
}

//==============================================================================
juce::AudioProcessorEditor* TensionProcessor::createEditor()
{
    return new TensionEditor (*this);
}

int TensionProcessor::getNumPrograms() { return kNumPresets; }

const juce::String TensionProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, kNumPresets) ? kPresets[index].name : "";
}

void TensionProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, kNumPresets))
        return;

    currentProgram = index;
    const auto& pr = kPresets[index];

    auto set = [this] (const char* id, float v)
    {
        if (auto* prm = apvts.getParameter (id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (v));
    };

    set (ParamID::curve, pr.curve);
    set (ParamID::rate, pr.rate);
    set (ParamID::filter, pr.filter);
    set (ParamID::noise, pr.noise);
    set (ParamID::stutter, pr.stutter);
    set (ParamID::reverb, pr.reverb);
    set (ParamID::drive, pr.drive);
    set (ParamID::width, pr.width);
    set (ParamID::impact, pr.impact);
    set (ParamID::mix, pr.mix);
}

void TensionProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void TensionProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            currentProgram = juce::jlimit (0, kNumPresets - 1, (int) state.getProperty ("program", 0));
            apvts.replaceState (state);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TensionProcessor();
}
