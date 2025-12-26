#include "ErodeLookAndFeel.h"

ErodeLookAndFeel::ErodeLookAndFeel() {
	setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff232323)); 
	setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffb3e5fc));
	setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff444444));
	setColour(juce::Slider::thumbColourId, juce::Colour(0xfffbc02d));
	setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff232323));
	setColour(juce::ComboBox::textColourId, juce::Colours::white);
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff232323));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff444444));
    setColour(juce::PopupMenu::textColourId, juce::Colours::white);
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::yellow);

}

void ErodeLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                       const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider)
{
    auto outline = slider.findColour (juce::Slider::rotarySliderOutlineColourId);
    auto fill    = slider.findColour (juce::Slider::rotarySliderFillColourId);

    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (10);

    auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = radius * 0.3f;
    auto arcRadius = radius - lineW * 0.5f;

    juce::Path backgroundArc;
    backgroundArc.addCentredArc (bounds.getCentreX(),
                                 bounds.getCentreY(),
                                 arcRadius,
                                 arcRadius,
                                 0.0f,
                                 rotaryStartAngle,
                                 rotaryEndAngle,
                                 true);

    g.setColour (outline);
    g.strokePath (backgroundArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (slider.isEnabled())
    {
        juce::Path valueArc;
        valueArc.addCentredArc (bounds.getCentreX(),
                                bounds.getCentreY(),
                                arcRadius,
                                arcRadius,
                                0.0f,
                                rotaryStartAngle,
                                toAngle,
                                true);

        g.setColour (fill);
        g.strokePath (valueArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}


juce::Label* ErodeLookAndFeel::createSliderTextBox(juce::Slider& slider) {
    auto* l = new juce::Label();
	float height = juce::jmax(slider.getHeight() * 0.2f, 15.0f);
    l->setFont(juce::Font(height));
    l->setJustificationType(juce::Justification::centred);
    l->setColour(juce::Label::backgroundColourId, juce::Colour(0xff232323));
    l->setColour(juce::Label::textColourId, juce::Colours::white);
    l->setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setBorderSize(juce::BorderSize<int>(6));
    return l;
}
