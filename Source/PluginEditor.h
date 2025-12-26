/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ErodeLookAndFeel.h"

//==============================================================================
/**
*/
class ErodeAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    ErodeAudioProcessorEditor (ErodeAudioProcessor&);
    ~ErodeAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ErodeAudioProcessor& audioProcessor;

    juce::Slider freqSlider;
	juce::Slider widthSlider;
	juce::Slider amountSlider;
	juce::Slider mixSlider;

	juce::ComboBox qualityBox;
	juce::ComboBox modeBox;

	juce::Label freqLabel;
	juce::Label widthLabel;
	juce::Label amountLabel;
	juce::Label mixLabel;
	juce::Label qualityLabel;
	juce::Label modeLabel;

	juce::AudioProcessorValueTreeState::SliderAttachment freqAttachment;
	juce::AudioProcessorValueTreeState::SliderAttachment widthAttachment;
	juce::AudioProcessorValueTreeState::SliderAttachment amountAttachment;
	juce::AudioProcessorValueTreeState::SliderAttachment mixAttachment;
	juce::AudioProcessorValueTreeState::ComboBoxAttachment qualityAttachment;
	juce::AudioProcessorValueTreeState::ComboBoxAttachment modeAttachment;

	ErodeLookAndFeel erodeLnf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ErodeAudioProcessorEditor)
};
