#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr float pi = juce::MathConstants<float>::pi;
    float dbToGain (float db) { return juce::Decibels::decibelsToGain (db); }
}

NEKODRIVEAudioProcessor::NEKODRIVEAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "NEKODRIVE_STATE", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout NEKODRIVEAudioProcessor::createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout p;
    auto add = [&p] (std::unique_ptr<RangedAudioParameter> param) { p.add (std::move (param)); };

    add (std::make_unique<AudioParameterFloat> ("gain", "Gain", NormalisableRange<float> (0.0f, 10.0f, 0.01f), 7.0f));
    add (std::make_unique<AudioParameterFloat> ("bass", "Bass", NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));
    add (std::make_unique<AudioParameterFloat> ("middle", "Middle", NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));
    add (std::make_unique<AudioParameterFloat> ("treble", "Treble", NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));
    add (std::make_unique<AudioParameterFloat> ("presence", "Presence", NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 2.0f));
    add (std::make_unique<AudioParameterFloat> ("master", "Master", NormalisableRange<float> (-36.0f, 6.0f, 0.1f), -6.0f));
    add (std::make_unique<AudioParameterFloat> ("gate", "Noise Gate", NormalisableRange<float> (-90.0f, -20.0f, 0.5f), -58.0f));
    add (std::make_unique<AudioParameterFloat> ("ampReverb", "Amp Reverb", NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.08f));
    add (std::make_unique<AudioParameterFloat> ("comp", "Compressor", NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    add (std::make_unique<AudioParameterFloat> ("compAttack", "Comp Attack", NormalisableRange<float> (1.0f, 100.0f, 0.1f), 15.0f));
    add (std::make_unique<AudioParameterFloat> ("compLevel", "Comp Level", NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));
    add (std::make_unique<AudioParameterFloat> ("pedalReverb", "Pedal Reverb", NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    add (std::make_unique<AudioParameterFloat> ("reverbDecay", "Reverb Decay", NormalisableRange<float> (0.1f, 0.98f, 0.01f), 0.55f));
    add (std::make_unique<AudioParameterFloat> ("doubler", "Stereo Doubler", NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    add (std::make_unique<AudioParameterFloat> ("doublerWidth", "Doubler Width", NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));
    add (std::make_unique<AudioParameterBool> ("cabinet", "Cabinet", true));
    add (std::make_unique<AudioParameterBool> ("tunerMute", "Tuner Mute", false));
    return p;
}

float NEKODRIVEAudioProcessor::readParam (const char* id) const noexcept
{
    if (auto* v = parameters.getRawParameterValue (id))
        return v->load();
    return 0.0f;
}

void NEKODRIVEAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, (juce::uint32) getTotalNumOutputChannels() };
    reverb.prepare (spec);
    compressor.prepare (spec);
    delayL.prepare (spec);
    delayR.prepare (spec);
    delayL.setMaximumDelayInSamples (4095);
    delayR.setMaximumDelayInSamples (4095);
    gateEnvelope = 0.0f;
    hpState[0] = hpState[1] = lpState[0] = lpState[1] = 0.0f;
}

void NEKODRIVEAudioProcessor::releaseResources() {}

bool NEKODRIVEAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return in == out;
}

void NEKODRIVEAudioProcessor::updateTuner (const juce::AudioBuffer<float>& buffer)
{
    // Lightweight zero-crossing estimate for the UI; not intended as a precision tuner yet.
    if (++tunerCounter < 4) return;
    tunerCounter = 0;
    if (buffer.getNumSamples() < 64) return;

    const auto* x = buffer.getReadPointer (0);
    int crossings = 0;
    float previous = x[0];
    for (int i = 1; i < buffer.getNumSamples(); ++i)
    {
        const float current = x[i];
        if (previous <= 0.0f && current > 0.0f) ++crossings;
        previous = current;
    }
    const float seconds = (float) buffer.getNumSamples() / (float) currentSampleRate;
    const float hz = seconds > 0.0f ? crossings / seconds : 0.0f;
    if (hz >= 20.0f && hz <= 1500.0f)
        tunerFrequency.store (hz);
}

void NEKODRIVEAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = getTotalNumInputChannels();
    const int samples = buffer.getNumSamples();
    const bool stereo = channels >= 2;
    lastInputWasStereo.store (stereo);

    for (int ch = getTotalNumOutputChannels(); ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, samples);

    updateTuner (buffer);

    const float gain = readParam ("gain");
    const float bass = readParam ("bass");
    const float mid = readParam ("middle");
    const float treble = readParam ("treble");
    const float presence = readParam ("presence");
    const float master = dbToGain (readParam ("master"));
    const float gateThreshold = dbToGain (readParam ("gate"));
    const float ampVerb = readParam ("ampReverb");
    const float compAmount = readParam ("comp");
    const float compLevel = dbToGain (readParam ("compLevel"));
    const float pedalVerb = readParam ("pedalReverb");
    const float doublerAmount = stereo ? readParam ("doubler") : 0.0f;
    const float width = readParam ("doublerWidth");
    const bool cabOn = readParam ("cabinet") > 0.5f;
    const bool tunerMute = readParam ("tunerMute") > 0.5f;

    compressor.setThreshold (-30.0f + (1.0f - compAmount) * 30.0f);
    compressor.setRatio (1.0f + compAmount * 9.0f);
    compressor.setAttack (readParam ("compAttack"));
    compressor.setRelease (100.0f);

    juce::dsp::Reverb::Parameters rp;
    rp.roomSize = juce::jlimit (0.1f, 0.98f, 0.35f + ampVerb * 0.5f + pedalVerb * 0.25f);
    rp.damping = 0.45f;
    rp.wetLevel = juce::jlimit (0.0f, 0.8f, ampVerb * 0.25f + pedalVerb * 0.55f);
    rp.dryLevel = 1.0f - rp.wetLevel * 0.5f;
    rp.width = 1.0f;
    rp.freezeMode = 0.0f;
    reverb.setParameters (rp);

    const float gateRelease = 0.9992f;
    const float gateAttack = 0.08f;
    const float bassGain = dbToGain (bass * 0.55f);
    const float midGain = dbToGain (mid * 0.55f);
    const float trebleGain = dbToGain (treble * 0.55f);
    const float presenceGain = dbToGain (presence * 0.45f);
    const float drive = 1.0f + gain * 2.8f;

    for (int ch = 0; ch < juce::jmin (channels, buffer.getNumChannels()); ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        for (int i = 0; i < samples; ++i)
        {
            const float in = data[i];
            const float magnitude = std::abs (in);
            gateEnvelope = magnitude > gateEnvelope ? gateEnvelope + gateAttack * (magnitude - gateEnvelope)
                                                     : gateEnvelope * gateRelease;
            const float gate = gateEnvelope < gateThreshold ? 0.0f : 1.0f;

            // Preamp saturation: asymmetric soft clipping for a modern British high-gain starting point.
            float x = in * drive;
            x = std::tanh (x * 1.35f) * 0.92f + 0.08f * std::tanh (x * 3.0f);
            x *= gate;

            // Simple musical tone stack approximation (low/high shelves + mid body).
            const float lowAlpha = 1.0f - std::exp (-2.0f * pi * 180.0f / (float) currentSampleRate);
            const float highAlpha = 1.0f - std::exp (-2.0f * pi * 2400.0f / (float) currentSampleRate);
            lpState[ch] += lowAlpha * (x - lpState[ch]);
            const float lowBand = lpState[ch];
            hpState[ch] += highAlpha * (x - hpState[ch]);
            const float highBand = x - hpState[ch];
            const float midBand = x - lowBand - highBand;
            x = lowBand * bassGain + midBand * midGain + highBand * trebleGain;
            x += highBand * (presenceGain - 1.0f);

            // Cabinet voicing approximation; a dedicated user IR loader is a planned next step.
            if (cabOn)
                x = std::tanh (x * 1.08f) * 0.82f;

            data[i] = x;
        }
    }

    if (compAmount > 0.001f)
    {
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> context (block);
        compressor.process (context);
        buffer.applyGain (compLevel);
    }

    if (stereo && doublerAmount > 0.001f && buffer.getNumChannels() >= 2)
    {
        auto* left = buffer.getWritePointer (0);
        auto* right = buffer.getWritePointer (1);
        const float delaySamplesL = (float) currentSampleRate * 0.012f;
        const float delaySamplesR = (float) currentSampleRate * 0.018f;
        delayL.setDelay (delaySamplesL);
        delayR.setDelay (delaySamplesR);
        for (int i = 0; i < samples; ++i)
        {
            const float l = left[i], r = right[i];
            delayL.pushSample (0, r);
            delayR.pushSample (0, l);
            const float wetL = delayL.popSample (0);
            const float wetR = delayR.popSample (0);
            left[i] = l * (1.0f - doublerAmount * 0.25f) + wetL * doublerAmount * 0.25f * width;
            right[i] = r * (1.0f - doublerAmount * 0.25f) + wetR * doublerAmount * 0.25f * width;
        }
    }

    if (ampVerb > 0.001f || pedalVerb > 0.001f)
    {
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> context (block);
        reverb.process (context);
    }

    buffer.applyGain (master);
    if (tunerMute)
        buffer.clear();
}

void NEKODRIVEAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void NEKODRIVEAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NEKODRIVEAudioProcessor();
}
