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
	qualityBox.setJustificationType(juce::Justification::centred);
	addAndMakeVisible(qualityBox);
	/*qualityLabel.setText("Quality", juce::dontSendNotification);
	qualityLabel.attachToComponent(&qualityBox, false);
	qualityLabel.setJustificationType(juce::Justification::centred);
	addAndMakeVisible(qualityLabel);*/

	modeBox.addItem("Noise", 1);
	modeBox.addItem("Sine", 2);
	modeBox.setSelectedId(1, juce::dontSendNotification);
	modeBox.setJustificationType(juce::Justification::centred);
	addAndMakeVisible(modeBox);
	/*modeLabel.setText("Mode", juce::dontSendNotification);
	modeLabel.attachToComponent(&modeBox, false);
	modeLabel.setJustificationType(juce::Justification::centred);
	addAndMakeVisible(modeLabel);*/

    setSize(400, 200);
    setResizable(true, true);
	setResizeLimits(400, 200, 1200, 600);
    getConstrainer()->setFixedAspectRatio(2.0);
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
	auto area = getLocalBounds().toFloat();

	// Margins
	float margin = 0.07f;
	area.reduce(area.getWidth() * margin, area.getHeight() * margin);

	// Row heights
	float sliderRowHeight = area.getHeight() * 0.6f;
	float comboRowHeight = area.getHeight() * 0.2f;

	// Top row: 4 sliders, evenly distributed
	auto sliderRow = area.removeFromTop(sliderRowHeight);
	float sliderPad = sliderRow.getWidth() * 0.025f;
	float sliderWidth = sliderRow.getWidth() / 4.0f;
	float sliderHeight = sliderRow.getHeight();

	for (int i = 0; i < 4; ++i)
	{
		auto col = sliderRow.withTrimmedLeft(i * sliderWidth).withWidth(sliderWidth);
		col = col.reduced(sliderPad, 0).withTrimmedTop(sliderPad * 2);

		switch (i)
		{
		case 0: freqSlider.setBounds(col.toNearestInt()); break;
		case 1: widthSlider.setBounds(col.toNearestInt()); break;
		case 2: amountSlider.setBounds(col.toNearestInt()); break;
		case 3: mixSlider.setBounds(col.toNearestInt()); break;
		}
	}

	// Center combo row in the remaining area
	float remainingHeight = area.getHeight();
	float comboRowY = area.getY() + (remainingHeight - comboRowHeight) / 2.0f;
	float comboPad = area.getWidth() * 0.025f;
	float comboWidth = area.getWidth() / 2.0f;

	juce::Rectangle<float> comboRow(area.getX(), comboRowY, area.getWidth(), comboRowHeight);

	auto leftCombo = comboRow.removeFromLeft(comboWidth).reduced(comboPad, 0);
	auto rightCombo = comboRow.reduced(comboPad, 0);

	qualityBox.setBounds(leftCombo.toNearestInt());
	modeBox.setBounds(rightCombo.toNearestInt());
}