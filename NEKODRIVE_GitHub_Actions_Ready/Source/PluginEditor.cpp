#include "PluginEditor.h"

NEKODRIVEAudioProcessorEditor::NEKODRIVEAudioProcessorEditor (NEKODRIVEAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (920, 650);
    title.setText ("NEKODRIVE", juce::dontSendNotification);
    title.setFont (juce::Font (juce::FontOptions (34.0f, juce::Font::bold)));
    title.setColour (juce::Label::textColourId, juce::Colour (0xffeeeaff));
    addAndMakeVisible (title);

    subtitle.setText ("CAT-INSPIRED MODERN HIGH-GAIN AMPLIFIER", juce::dontSendNotification);
    subtitle.setFont (juce::Font (juce::FontOptions (11.0f)));
    subtitle.setColour (juce::Label::textColourId, juce::Colour (0xffa99bd8));
    addAndMakeVisible (subtitle);

    const std::pair<juce::Slider*, juce::String> controls[] = {
        { &gain, "GAIN" }, { &bass, "BASS" }, { &middle, "MIDDLE" },
        { &treble, "TREBLE" }, { &presence, "PRESENCE" }, { &master, "MASTER" },
        { &gate, "NOISE GATE" }, { &ampReverb, "AMP REVERB" },
        { &compressor, "COMPRESSOR" }, { &pedalReverb, "PEDAL REVERB" },
        { &doubler, "STEREO DOUBLER" }, { &width, "WIDTH" }
    };

    for (auto item : controls)
    {
        setupSlider (*item.first, item.second);
        addAndMakeVisible (*item.first);
    }

    attach (gain, "gain"); attach (bass, "bass"); attach (middle, "middle");
    attach (treble, "treble"); attach (presence, "presence"); attach (master, "master");
    attach (gate, "gate"); attach (ampReverb, "ampReverb");
    attach (compressor, "comp"); attach (pedalReverb, "pedalReverb");
    attach (doubler, "doubler"); attach (width, "doublerWidth");

    cabinet.setButtonText ("CABINET ON");
    addAndMakeVisible (cabinet);
    cabinetAttachment = std::make_unique<ButtonAttachment> (audioProcessor.parameters, "cabinet", cabinet);
    tunerMute.setButtonText ("TUNER MUTE");
    addAndMakeVisible (tunerMute);
    tunerMuteAttachment = std::make_unique<ButtonAttachment> (audioProcessor.parameters, "tunerMute", tunerMute);

    presetBox.addItem ("Default", 1);
    presetBox.addItem ("British Bite", 2);
    presetBox.addItem ("Tight Modern", 3);
    presetBox.setSelectedId (1);
    addAndMakeVisible (presetBox);

    tunerReadout.setText ("TUNER  --.- Hz", juce::dontSendNotification);
    tunerReadout.setColour (juce::Label::textColourId, juce::Colour (0xff67f4d0));
    tunerReadout.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));
    addAndMakeVisible (tunerReadout);

    savePreset.onClick = [this]
    {
        juce::FileChooser chooser ("Save NEKODRIVE preset", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory), "*.nekodrive");
        if (chooser.browseForFileToSave (true))
        {
            auto state = audioProcessor.parameters.copyState();
            if (auto xml = state.createXml())
                xml->writeTo (chooser.getResult());
        }
    };
    addAndMakeVisible (savePreset);

    startTimerHz (8);
}

void NEKODRIVEAudioProcessorEditor::setupSlider (juce::Slider& s, const juce::String& name)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 18);
    s.setName (name);
    s.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffa78bfa));
    s.setColour (juce::Slider::thumbColourId, juce::Colour (0xff67e8f9));
    s.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff39334e));
}

void NEKODRIVEAudioProcessorEditor::attach (juce::Slider& s, const juce::String& id)
{
    sliderAttachments.push_back (std::make_unique<SliderAttachment> (audioProcessor.parameters, id, s));
}

void NEKODRIVEAudioProcessorEditor::timerCallback()
{
    const float hz = audioProcessor.getTunerFrequency();
    tunerReadout.setText (hz > 0.0f ? "TUNER  " + juce::String (hz, 1) + " Hz" : "TUNER  --.- Hz",
                          juce::dontSendNotification);
    doubler.setEnabled (audioProcessor.isStereoInput());
    width.setEnabled (audioProcessor.isStereoInput());
}

void NEKODRIVEAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff090812));
    g.setColour (juce::Colour (0xff211735));
    g.fillRoundedRectangle (12.0f, 12.0f, (float) getWidth() - 24.0f, (float) getHeight() - 24.0f, 18.0f);
    g.setColour (juce::Colour (0xff8b5cf6));
    g.drawRoundedRectangle (12.0f, 12.0f, (float) getWidth() - 24.0f, (float) getHeight() - 24.0f, 18.0f, 2.0f);

    // Minimal cat mascot silhouette, drawn in code so no external image assets are required.
    juce::Path cat;
    cat.startNewSubPath (65.0f, 104.0f);
    cat.lineTo (72.0f, 58.0f); cat.lineTo (104.0f, 83.0f);
    cat.quadraticTo (145.0f, 66.0f, 180.0f, 83.0f);
    cat.lineTo (212.0f, 58.0f); cat.lineTo (220.0f, 104.0f);
    cat.quadraticTo (246.0f, 170.0f, 143.0f, 174.0f);
    cat.quadraticTo (42.0f, 170.0f, 65.0f, 104.0f);
    cat.closeSubPath();
    g.setColour (juce::Colour (0xfff0eef8));
    g.fillPath (cat);
    g.setColour (juce::Colour (0xff171321));
    g.fillEllipse (103.0f, 112.0f, 9.0f, 11.0f);
    g.fillEllipse (172.0f, 112.0f, 9.0f, 11.0f);
    g.drawLine (142.0f, 128.0f, 142.0f, 137.0f, 2.0f);
    g.drawLine (142.0f, 137.0f, 132.0f, 143.0f, 2.0f);
    g.drawLine (142.0f, 137.0f, 152.0f, 143.0f, 2.0f);

    g.setColour (juce::Colour (0xffa78bfa));
    g.drawLine (28.0f, 204.0f, (float) getWidth() - 28.0f, 204.0f, 1.5f);
    g.setColour (juce::Colour (0xffc4b5fd));
    g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    g.drawText ("MARSHALL-INSPIRED MODERN HIGH GAIN  |  MONO / STEREO", 28, 614, getWidth() - 56, 20, juce::Justification::centred);
}

void NEKODRIVEAudioProcessorEditor::resized()
{
    title.setBounds (260, 30, 360, 45);
    subtitle.setBounds (263, 75, 420, 20);
    presetBox.setBounds (700, 42, 180, 28);

    const int topY = 220, cellW = 142, cellH = 174;
    juce::Slider* top[] = { &gain, &bass, &middle, &treble, &presence, &master };
    for (int i = 0; i < 6; ++i)
        top[i]->setBounds (28 + i * cellW, topY, 130, cellH);

    const int bottomY = 410;
    compressor.setBounds (28, bottomY, 130, 150);
    pedalReverb.setBounds (170, bottomY, 130, 150);
    doubler.setBounds (312, bottomY, 130, 150);
    width.setBounds (454, bottomY, 130, 150);
    gate.setBounds (596, bottomY, 130, 150);
    ampReverb.setBounds (738, bottomY, 130, 150);

    cabinet.setBounds (610, 178, 120, 26);
    tunerMute.setBounds (28, 178, 120, 26);
    tunerReadout.setBounds (300, 150, 220, 28);
    savePreset.setBounds (750, 178, 120, 26);
}
