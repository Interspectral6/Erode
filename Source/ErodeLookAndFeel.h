#pragma once
#include <JuceHeader.h>

class ErodeLookAndFeel : public juce::LookAndFeel_V4
{
public:
	ErodeLookAndFeel();

	void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
		float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
		juce::Slider& slider) override;

	/*void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;*/

	void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
		bool isSeparator, bool isActive, bool isHighlighted,
		bool /*isTicked*/, bool /*hasSubMenu*/,
		const juce::String& text, const juce::String&,
		const juce::Drawable*, const juce::Colour* textColour) override;

	juce::Label* createSliderTextBox(juce::Slider& slider) override;

	juce::Font getComboBoxFont(juce::ComboBox& box) override;
};
