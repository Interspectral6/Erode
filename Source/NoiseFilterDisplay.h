#pragma once
#include <JuceHeader.h>

class NoiseFilterDisplay : public juce::Component, private juce::Timer
{
public:
    NoiseFilterDisplay(juce::AudioProcessorValueTreeState& apvts);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvts;
    void timerCallback() override;
};