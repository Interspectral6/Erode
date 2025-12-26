#pragma once
#include <JuceHeader.h>

class ErodeLookAndFeel : public juce::LookAndFeel_V4
{
public:
	ErodeLookAndFeel();

	void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
		float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
		juce::Slider& slider) override;

	juce::Label* createSliderTextBox(juce::Slider& slider) override;
};
