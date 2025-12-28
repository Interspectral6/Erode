#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class NoiseFilterDisplay : public juce::Component, private juce::Timer
{
public:
    NoiseFilterDisplay(ErodeAudioProcessor& p, juce::AudioProcessorValueTreeState& apvts);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvts;
    ErodeAudioProcessor& p;

	juce::dsp::FFT fft;
    std::vector<float> outfftInput;
    std::vector<float> outfftData;
    std::vector<float> outMagnitudes;
    std::vector<float> infftInput;
    std::vector<float> infftData;
    std::vector<float> inMagnitudes;
    juce::dsp::WindowingFunction<float> window;
    juce::Point<float> dragStart;
    float startFreq = 0.0f;
	float startWidth = 0.0f;
    bool draggingBand = false;

    void timerCallback() override;
	void mouseDown(const juce::MouseEvent& e) override;
	void mouseDrag(const juce::MouseEvent& e) override;
	void mouseUp(const juce::MouseEvent& e) override;
};