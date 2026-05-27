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
    feedbackAttachment(p.getAPVTS(), "feedback", feedbackSlider),
	cutAttachment(p.getAPVTS(), "cut", cutSlider),
	filterDisplay(p, p.getAPVTS())
{
    // Each slider is attached directly to APVTS so host automation, preset state,
    // and UI edits all share the same parameter source.
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

    feedbackSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    feedbackSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    feedbackSlider.setTooltip("Amount of delayed signal fed back into the delay input");
    addAndMakeVisible(feedbackSlider);
    feedbackLabel.setText("Feedback", juce::dontSendNotification);
    feedbackLabel.attachToComponent(&feedbackSlider, false);
    feedbackLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(feedbackLabel);

	cutSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    cutSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
	addAndMakeVisible(cutSlider);
	cutLabel.setText("Cut", juce::dontSendNotification);
	cutLabel.attachToComponent(&cutSlider, false);
	cutLabel.setJustificationType(juce::Justification::centred);
	addAndMakeVisible(cutLabel);
	addAndMakeVisible(filterDisplay);

	setLookAndFeel(&erodeLnf);

    // Fixed aspect ratio keeps the spectrum and five-knob layout readable while
    // still allowing hosts to resize the plugin window.
    setSize(500, 200);
    setResizable(true, true);
	setResizeLimits(500, 200, 1400, 560);
    getConstrainer()->setFixedAspectRatio(2.5);
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
	auto area = getLocalBounds().toFloat();

    // Top strip is the interactive spectrum/filter display; bottom area is the
    // parameter control row.
	filterDisplay.setBounds(area.removeFromTop(getHeight() * 0.4f).toNearestInt());

    // Scale text boxes and labels with window height so the UI remains legible
    // at the minimum and maximum resize limits.
	int textBoxWidth = juce::roundToInt(getWidth() * 0.14f);
	int textBoxHeight = juce::roundToInt(getHeight() * 0.1f);
	for (auto* s : { &freqSlider, &widthSlider, &amountSlider, &feedbackSlider, &cutSlider })
		s->setTextBoxStyle(juce::Slider::TextBoxBelow, false, textBoxWidth, textBoxHeight);
	
	float fontSize = getHeight() * 0.08f;
	for (auto* l : { &freqLabel, &widthLabel, &amountLabel, &feedbackLabel, &cutLabel })
		l->setFont(juce::Font(juce::FontOptions(fontSize)));

	float margin = 0.07f;
	area.reduce(area.getWidth() * margin, area.getHeight() * margin * 2);

	float sliderPad = area.getWidth() * 0.025f;
	float sliderWidth = area.getWidth() / 5.0f;

    // Five equal columns, one per parameter.
	for (int i = 0; i < 5; ++i)
	{
		auto col = area.withTrimmedLeft(i * sliderWidth).withWidth(sliderWidth);
		col = col.reduced(sliderPad, 0).withTrimmedTop(sliderPad * 2);

		switch (i)
		{
		case 0: freqSlider.setBounds(col.toNearestInt()); break;
		case 1: widthSlider.setBounds(col.toNearestInt()); break;
		case 2: amountSlider.setBounds(col.toNearestInt()); break;
        case 3: feedbackSlider.setBounds(col.toNearestInt()); break;
		case 4: cutSlider.setBounds(col.toNearestInt()); break;
		}
	}
}
