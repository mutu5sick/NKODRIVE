#pragma once
#include <JuceHeader.h>

class NEKODRIVEAudioProcessor : public juce::AudioProcessor
{
public:
    NEKODRIVEAudioProcessor();
    ~NEKODRIVEAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;
    float getTunerFrequency() const noexcept { return tunerFrequency.load(); }
    bool isStereoInput() const noexcept { return lastInputWasStereo.load(); }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    juce::dsp::Reverb reverb;
    juce::dsp::Compressor<float> compressor;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayL { 4096 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayR { 4096 };

    double currentSampleRate = 44100.0;
    float gateEnvelope = 0.0f;
    float hpState[2] { 0.0f, 0.0f };
    float lpState[2] { 0.0f, 0.0f };
    std::atomic<float> tunerFrequency { 0.0f };
    std::atomic<bool> lastInputWasStereo { false };
    int tunerCounter = 0;

    float readParam (const char* id) const noexcept;
    void updateTuner (const juce::AudioBuffer<float>& buffer);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NEKODRIVEAudioProcessor)
};
