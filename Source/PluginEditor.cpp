/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ErodeAudioProcessorEditor::ErodeAudioProcessorEditor (ErodeAudioProcessor& p)
	: AudioProcessorEditor(&p), audioProcessor(p), 
    freqAttachment(p.getAPVTS(), "freq", freqSlider),
	widthAttachment(p.getAPVTS(), "width", widthSlider),
	amountAttachment(p.getAPVTS(), "amount", amountSlider),
	mixAttachment(p.getAPVTS(), "mix", mixSlider),
	qualityAttachment(p.getAPVTS(), "quality", qualityBox),
	modeAttachment(p.getAPVTS(), "mode", modeBox)
{
    freqSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    freqSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
	addAndMakeVisible(freqSlider);
	freqLabel.setText("Freq", juce::dontSendNotification);
	freqLabel.attachToComponent(&freqSlider, false);
	freqLabel.setJustificationType(juce::Justification::centred);
	addAndMakeVisible(freqLabel);

	widthSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
	widthSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
	addAndMakeVisible(widthSlider);
	widthLabel.setText("Width", juce::dontSendNotification);
	widthLabel.attachToComponent(&widthSlider, false);
	widthLabel.setJustificationType(juce::Justification::centred);
	addAndMakeVisible(widthLabel);

	amountSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    amountSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
	addAndMakeVisible(amountSlider);
	amountLabel.setText("Amount", juce::dontSendNotification);
	amountLabel.attachToComponent(&amountSlider, false);
	amountLabel.setJustificationType(juce::Justification::centred);
	addAndMakeVisible(amountLabel);

	mixSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
	addAndMakeVisible(mixSlider);
	mixLabel.setText("Mix", juce::dontSendNotification);
	mixLabel.attachToComponent(&mixSlider, false);
	mixLabel.setJustificationType(juce::Justification::centred);
	addAndMakeVisible(mixLabel);

	qualityBox.addItem("Smooth", 1);
	qualityBox.addItem("Rough", 2);
	qualityBox.setSelectedId(1, juce::dontSendNotification);
	addAndMakeVisible(qualityBox);
	qualityLabel.setText("Quality", juce::dontSendNotification);
	qualityLabel.attachToComponent(&qualityBox, false);
	qualityLabel.setJustificationType(juce::Justification::centred);
	addAndMakeVisible(qualityLabel);

	modeBox.addItem("Noise", 1);
	modeBox.addItem("Sine", 2);
	modeBox.setSelectedId(1, juce::dontSendNotification);
	addAndMakeVisible(modeBox);
	modeLabel.setText("Mode", juce::dontSendNotification);
	modeLabel.attachToComponent(&modeBox, false);
	modeLabel.setJustificationType(juce::Justification::centred);
	addAndMakeVisible(modeLabel);

    setSize (400, 300);
}

ErodeAudioProcessorEditor::~ErodeAudioProcessorEditor()
{
}

//==============================================================================
void ErodeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void ErodeAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
	juce::Rectangle bounds = getLocalBounds().reduced(20);
	juce::Rectangle topRow = bounds.removeFromTop(120);

	int sliderWidth = topRow.getWidth() / 4;

	freqSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(10).withTrimmedTop (15));
	widthSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(10).withTrimmedTop (15));
	amountSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(10).withTrimmedTop (15));
	mixSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(10).withTrimmedTop (15));

	juce::Rectangle bottomRow = bounds.removeFromTop(60);
	int comboWidth = bottomRow.getWidth() / 2;

	qualityBox.setBounds(bottomRow.removeFromLeft(comboWidth).reduced(10).withTrimmedTop(15));
	modeBox.setBounds(bottomRow.removeFromLeft(comboWidth).reduced(10).withTrimmedTop(15));

}

