/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ErodeAudioProcessorEditor::ErodeAudioProcessorEditor (ErodeAudioProcessor& p)
	: AudioProcessorEditor(&p), audioProcessor(p), tolltipWindow(this),
    freqAttachment(p.getAPVTS(), "freq", freqSlider),
	widthAttachment(p.getAPVTS(), "width", widthSlider),
	amountAttachment(p.getAPVTS(), "amount", amountSlider),
	cutAttachment(p.getAPVTS(), "cut", cutSlider),
	filterDisplay(p, p.getAPVTS())
{
    freqSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    freqSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
	freqSlider.setTooltip("Center frequency of the modulator");
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

	cutSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    cutSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
	addAndMakeVisible(cutSlider);
	cutLabel.setText("Cut", juce::dontSendNotification);
	cutLabel.attachToComponent(&cutSlider, false);
	cutLabel.setJustificationType(juce::Justification::centred);
	addAndMakeVisible(cutLabel);
	addAndMakeVisible(filterDisplay);

	setLookAndFeel(&erodeLnf);
    setSize(400, 200);
    setResizable(true, true);
	setResizeLimits(400, 200, 1200, 600);
    getConstrainer()->setFixedAspectRatio(2.0);
}

ErodeAudioProcessorEditor::~ErodeAudioProcessorEditor()
{
	setLookAndFeel(nullptr);
}

//==============================================================================
void ErodeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void ErodeAudioProcessorEditor::resized()
{
	DBG("resized");
	auto area = getLocalBounds().toFloat();
	filterDisplay.setBounds(area.removeFromTop(getHeight() * 0.4f).toNearestInt());

	int textBoxWidth = getWidth() * 0.14f;
	int textBoxHeight = getHeight() * 0.1f;
	for (auto* s : { &freqSlider, &widthSlider, &amountSlider, &cutSlider })
		s->setTextBoxStyle(juce::Slider::TextBoxBelow, false, textBoxWidth, textBoxHeight);
	
	float fontSize = getHeight() * 0.08f;
	for (auto* l : { &freqLabel, &widthLabel, &amountLabel, &cutLabel })
		l->setFont(juce::Font(fontSize));

	float margin = 0.07f;
	area.reduce(area.getWidth() * margin, area.getHeight() * margin * 2);

	float sliderPad = area.getWidth() * 0.025f;
	float sliderWidth = area.getWidth() / 4.0f;
	float sliderHeight = area.getHeight();

	for (int i = 0; i < 4; ++i)
	{
		auto col = area.withTrimmedLeft(i * sliderWidth).withWidth(sliderWidth);
		col = col.reduced(sliderPad, 0).withTrimmedTop(sliderPad * 2);

		switch (i)
		{
		case 0: freqSlider.setBounds(col.toNearestInt()); break;
		case 1: widthSlider.setBounds(col.toNearestInt()); break;
		case 2: amountSlider.setBounds(col.toNearestInt()); break;
		case 3: cutSlider.setBounds(col.toNearestInt()); break;
		}
	}
}
