#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class NEKODRIVEAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit NEKODRIVEAudioProcessorEditor (NEKODRIVEAudioProcessor&);
    ~NEKODRIVEAudioProcessorEditor() override = default;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    NEKODRIVEAudioProcessor& audioProcessor;
    juce::Label title, subtitle, tunerReadout;
    juce::Slider gain, bass, middle, treble, presence, master, gate, ampReverb;
    juce::Slider compressor, pedalReverb, doubler, width;
    juce::ToggleButton cabinet, tunerMute;
    juce::ComboBox presetBox;
    std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
    std::unique_ptr<ButtonAttachment> cabinetAttachment, tunerMuteAttachment;
    juce::TextButton savePreset { "SAVE PRESET" };

    void setupSlider (juce::Slider&, const juce::String&);
    void attach (juce::Slider&, const juce::String&);
    void timerCallback();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NEKODRIVEAudioProcessorEditor)
};
