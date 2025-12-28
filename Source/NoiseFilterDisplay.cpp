#include "NoiseFilterDisplay.h"

NoiseFilterDisplay::NoiseFilterDisplay(ErodeAudioProcessor& p, juce::AudioProcessorValueTreeState& state) :
    apvts(state), p(p),
    fft(p.fftOrder),
    outfftInput(p.fftSize, 0.0f),
    outfftData(p.fftSize * 2, 0.0f), 
    outMagnitudes(p.fftSize / 2, 0.0f),
    infftInput(p.fftSize, 0.0f),
    infftData(p.fftSize * 2, 0.0f), 
    inMagnitudes(p.fftSize / 2, 0.0f),
	window(p.fftSize, juce::dsp::WindowingFunction<float>::hann)
{
    startTimerHz(60);
	setMouseClickGrabsKeyboardFocus(false);
	setWantsKeyboardFocus(false);
}

void NoiseFilterDisplay::timerCallback()
{
    // Perform FFT for spectrum display
    int outputWritePos = p.outputWritePos;
    auto& outputBuffer = p.outputBuffer;
    for (int i = 0; i < p.fftSize; ++i) {
        outfftInput[i] = outputBuffer.getSample(0, (outputWritePos + i) % p.fftSize);
    }
	window.multiplyWithWindowingTable(outfftInput.data(), p.fftSize);
    std::fill(outfftData.begin(), outfftData.end(), 0.0f);
	std::copy(outfftInput.begin(), outfftInput.end(), outfftData.begin());
    fft.performRealOnlyForwardTransform(outfftData.data(), true);
    for (int i = 0; i < p.fftSize / 2; ++i) {
        float re = outfftData[2 * i];
        float im = outfftData[2 * i + 1];
		float mag = std::sqrt(re * re + im * im);
        outMagnitudes[i] = juce::jmax(mag, outMagnitudes[i] * 0.97f); // peak hold smoothing
    }

    int inputWritePos = p.inputWritePos;
    auto& inputBuffer = p.inputBuffer;
    for (int i = 0; i < p.fftSize; ++i) {
        infftInput[i] = inputBuffer.getSample(0, (inputWritePos + i) % p.fftSize);
    }
	window.multiplyWithWindowingTable(infftInput.data(), p.fftSize);
    std::fill(infftData.begin(), infftData.end(), 0.0f);
	std::copy(infftInput.begin(), infftInput.end(), infftData.begin());
    fft.performRealOnlyForwardTransform(infftData.data(), true);
    for (int i = 0; i < p.fftSize / 2; ++i) {
        float re = infftData[2 * i];
        float im = infftData[2 * i + 1];
		float mag = std::sqrt(re * re + im * im);
        inMagnitudes[i] = juce::jmax(mag, inMagnitudes[i] * 0.97f);    
    }
    repaint();
}

void NoiseFilterDisplay::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    g.fillAll(juce::Colours::black.withAlpha(0.7f));

    // Draw output
    float binFreq = p.getSampleRate() / (float)p.fftSize;
    auto freqToX = [area, binFreq](float hz) {
        float norm = std::log10(hz / 20.0f) / std::log10(20000.0f / 20.0f);
        return area.getX() + norm * area.getWidth();
		};
    float maxDb = -20.0f, minDb = -120.0f;
    auto magToY = [area, minDb, maxDb](float mag) {
        float db = juce::Decibels::gainToDecibels(mag, minDb);
        float norm = juce::jlimit(0.0f, 1.0f, (db - minDb) / (maxDb - minDb));
        return area.getHeight() * (1.0f - norm);
        };

	g.setColour(juce::Colours::white.withAlpha(0.7f));
    juce::Path outputPath;
    outputPath.startNewSubPath(0, magToY(outMagnitudes[0] / p.fftSize));
    for (int i = 1; i < (int)outMagnitudes.size(); ++i) {
		if (binFreq * i < 20.0f || binFreq * i > 20000.0f)
            continue;
        outputPath.lineTo(freqToX(binFreq * i), magToY(outMagnitudes[i] / p.fftSize));
    }
    g.strokePath(outputPath, juce::PathStrokeType(2.0f));

	g.setColour(juce::Colours::white.withAlpha(0.4f));
    juce::Path inputPath;
    inputPath.startNewSubPath(0, magToY(inMagnitudes[0] / p.fftSize));
    for (int i = 1; i < (int)inMagnitudes.size(); ++i) {
		if (binFreq * i < 20.0f || binFreq * i > 20000.0f)
            continue;
        inputPath.lineTo(freqToX(binFreq * i), magToY(inMagnitudes[i] / p.fftSize));
    }
    g.strokePath(inputPath, juce::PathStrokeType(2.0f));

    float freq = apvts.getRawParameterValue("freq")->load();
    float width = apvts.getRawParameterValue("width")->load();
	float amount = apvts.getRawParameterValue("amount")->load();

    // Map freq (20Hz-20kHz) to X
    float centerX = freqToX(freq);
    float bandWidth = area.getWidth() * juce::jmap(width, 0.0f, 1.0f, 0.01f, 0.5f);

    // Draw bandpass region
    g.setColour(juce::Colours::deepskyblue.withAlpha(0.5f + (amount - 0.5f) * 0.3f));
    g.fillRect(centerX - bandWidth * 0.5f, area.getY(), bandWidth, area.getHeight());
}

void NoiseFilterDisplay::resized()
{
    // No child components
}

void NoiseFilterDisplay::mouseDown(const juce::MouseEvent& e)
{
    // Get current freq/width
    float freq = apvts.getRawParameterValue("freq")->load();
    float width = apvts.getRawParameterValue("width")->load();

    // Calculate band rectangle
    auto area = getLocalBounds().toFloat();
    auto freqToX = [area](float hz) {
        float norm = std::log10(hz / 20.0f) / std::log10(20000.0f / 20.0f);
        return area.getX() + norm * area.getWidth();
        };
    float centerX = freqToX(freq);
    float bandWidth = area.getWidth() * juce::jmap(width, 0.0f, 1.0f, 0.01f, 0.5f);
    juce::Rectangle<float> bandRect(centerX - bandWidth * 0.5f, area.getY(), bandWidth, area.getHeight());

    if (bandRect.contains(e.position))
    {
        draggingBand = true;
        dragStart = e.position;
        startFreq = freq;
        startWidth = width;
    }
}

void NoiseFilterDisplay::mouseDrag(const juce::MouseEvent& e)
{
    if (!draggingBand)
        return;

    auto area = getLocalBounds().toFloat();

    // Horizontal drag: freq
    float dx = e.position.x - dragStart.x;
    float freqNorm = std::log10(startFreq / 20.0f) / std::log10(20000.0f / 20.0f);
    freqNorm += dx / area.getWidth();
    freqNorm = juce::jlimit(0.0f, 1.0f, freqNorm);
    float newFreq = 20.0f * std::pow(20000.0f / 20.0f, freqNorm);

    // Vertical drag: width
    float dy = e.position.y - dragStart.y;
    float newWidth = juce::jlimit(0.0f, 1.0f, startWidth - dy / area.getHeight());

    auto* freqParam = apvts.getParameter("freq");
    auto* widthParam = apvts.getParameter("width");
    if (freqParam)
        freqParam->setValueNotifyingHost(freqParam->convertTo0to1(newFreq));
    if (widthParam)
        widthParam->setValueNotifyingHost(widthParam->convertTo0to1(newWidth));
}

void NoiseFilterDisplay::mouseUp(const juce::MouseEvent&)
{
    draggingBand = false;
}