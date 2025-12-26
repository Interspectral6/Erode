#include "NoiseFilterDisplay.h"

NoiseFilterDisplay::NoiseFilterDisplay(juce::AudioProcessorValueTreeState& state) : apvts(state)
{
    startTimerHz(30); // update at 30 FPS
}

void NoiseFilterDisplay::timerCallback()
{
    repaint();
}

void NoiseFilterDisplay::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    g.fillAll(juce::Colours::black.withAlpha(0.7f));

    // Get parameters
    float freq = apvts.getRawParameterValue("freq")->load();
    float width = apvts.getRawParameterValue("width")->load();

    // Draw frequency axis
    g.setColour(juce::Colours::grey);
    g.drawLine(area.getX(), area.getCentreY(), area.getRight(), area.getCentreY(), 1.0f);

    // Map freq (20Hz-20kHz) to X
    auto freqToX = [area](float hz) {
        float norm = std::log10(hz / 20.0f) / std::log10(20000.0f / 20.0f);
        return area.getX() + norm * area.getWidth();
        };

    float centerX = freqToX(freq);
    float bandWidth = area.getWidth() * juce::jmap(width, 0.0f, 1.0f, 0.01f, 0.5f);

    // Draw bandpass region
    g.setColour(juce::Colours::deepskyblue.withAlpha(0.5f));
    g.fillRect(centerX - bandWidth * 0.5f, area.getY(), bandWidth, area.getHeight());
}

void NoiseFilterDisplay::resized()
{
    // No child components
}