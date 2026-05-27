/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

constexpr float twoPi = juce::MathConstants<float>::twoPi;
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout ErodeAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Main modulation frequency. Exponential skew gives useful resolution in
    // low frequencies while still covering the full audible range.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "freq",
        "Freq",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f),
        1000.0f));
    
    // Width controls two things at once:
    // - filter Q for the noise modulator
    // - crossfade between sine-like and noise-like delay modulation
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "width",
        "Width",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f));

    // Amount acts as both wet/dry mix and modulation depth.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "amount",
        "Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f));

    // Feedback returns delayed signal to the delay input. It is capped below
    // unity to reduce runaway gain in the modulated delay loop.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "feedback",
        "Feedback",
        juce::NormalisableRange<float>(0.0f, 0.95f, 0.01f),
        0.0f));

    // Output high-pass cleanup after the modulated delay stage.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "cut",
        "Cut",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f),
        20.0f));
    return layout;
}

ErodeAudioProcessor::ErodeAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
    apvts (*this, nullptr, "Parameters", createParameterLayout())
#endif
{
}

ErodeAudioProcessor::~ErodeAudioProcessor()
{
}

//==============================================================================
const juce::String ErodeAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ErodeAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ErodeAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ErodeAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ErodeAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ErodeAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ErodeAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ErodeAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused(index);
}

const juce::String ErodeAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused(index);
    return {};
}

void ErodeAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void ErodeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // The effect currently uses a very short fixed delay window. This is enough
    // for the erosion-style phase/pitch modulation without sounding like an echo.
    delayBuffer.setSize(getTotalNumOutputChannels(), 100);
    delayBuffer.clear();
	
    writePosition = 0;
    lfoPhase = 0.0f;

    // Band-pass filter shapes white noise into a narrow or wide modulator band.
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    filter.prepare(spec);
    filter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    outputHPF.resize(getTotalNumInputChannels());
    for (auto& hpf : outputHPF) {
		hpf.reset();
		hpf.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 2000.0f, 0.5f);
    }
 
    // Circular mono buffers feed the editor spectrum display. Audio thread writes
    // them continuously; UI thread reads snapshots in NoiseFilterDisplay.
	outputBuffer.setSize(1, fftSize);
    outputBuffer.clear();
    outputWritePos = 0;
	inputBuffer.setSize(1, fftSize);
    inputBuffer.clear();
	inputWritePos = 0;

	smoothedAmount.reset(sampleRate, 0.05);
	smoothedAmount.setCurrentAndTargetValue(apvts.getRawParameterValue("amount")->load());

    smoothedFeedback.reset(sampleRate, 0.05);
    smoothedFeedback.setCurrentAndTargetValue(apvts.getRawParameterValue("feedback")->load());

    smoothedCut.reset(sampleRate, 0.05);
    smoothedCut.setCurrentAndTargetValue(apvts.getRawParameterValue("cut")->load());
}

void ErodeAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ErodeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
      if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void ErodeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    const int numSamples = buffer.getNumSamples();
    const int bufferSize = delayBuffer.getNumSamples();
    const float sampleRate = static_cast<float>(getSampleRate());
    float offset = 0.0f;
    float noise = 0.0f;
    float sine = 0.0f;
	smoothedAmount.setTargetValue(apvts.getRawParameterValue("amount")->load());
	float sAmount = smoothedAmount.getNextValue();
	float mix = sAmount;
    float amount = mix * 20.0f;
    smoothedFeedback.setTargetValue(apvts.getRawParameterValue("feedback")->load());
    float feedback = smoothedFeedback.getNextValue();
    int delayInSamples = 30;
    float freq = apvts.getRawParameterValue("freq")->load();
    float width = apvts.getRawParameterValue("width")->load();

    // Width maps inversely to Q: narrow width gives a resonant, sine-like band;
    // wide width gives broadband noisy modulation.
    float minQ = 0.5f;
    float maxQ = 30.0f;
    float q = minQ * std::pow(maxQ / minQ, 1.0f - width);

    // Perceptual crossfade. Width near 0 favors sine; width near 1 favors noise.
    float sineAmount = 1.0f - std::pow(width, 0.7f); // Lower coefficient means less sine
    float noiseAmount = 1.0f - sineAmount;
	smoothedCut.setTargetValue(apvts.getRawParameterValue("cut")->load());
	float sCut = smoothedCut.getNextValue();
    for (auto& hpf : outputHPF) {
		hpf.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, sCut, 0.5f);
    }

	filter.setCutoffFrequency(freq);
	filter.setResonance(q);

    float outputMonoSum = 0.0f;
	float inputMonoSum = 0.0f;

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, numSamples);

    for (int sample = 0; sample < numSamples; ++sample) {
        // Update smoothed parameters per sample so automation changes do not
        // click, especially for mix/depth and high-pass cutoff.
		sAmount = smoothedAmount.getNextValue();
		mix = sAmount;
		amount = mix * 20.0f;
        feedback = smoothedFeedback.getNextValue();

		sCut = smoothedCut.getNextValue();
        for (auto& hpf : outputHPF) {
			hpf.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, sCut, 0.5f);
        }

		noise = rand.nextFloat() * 2.0f - 1.0f;
        
        // std::pow here is to balance the loudness of noise, since higher q means louder
		noise = filter.processSample(0, noise) * std::pow(width, 0.2f); // Lower coefficient means more noise
		noise = std::tanh(noise);

		sine = std::sin(lfoPhase);
		lfoPhase += twoPi * freq / sampleRate;
		if (lfoPhase >= twoPi) lfoPhase -= twoPi;

        // Crossfade between noise and sine
        offset = noiseAmount * noise + sineAmount * sine;

        // Read from a fixed short delay, then push/pull the read position by the
        // modulator. Fractional interpolation avoids coarse zipper artifacts.
		float readPosition = writePosition - delayInSamples + offset * amount;
		while (readPosition < 0) readPosition += bufferSize;
		while (readPosition >= bufferSize) readPosition -= bufferSize;
		int index0 = static_cast<int>(readPosition);
		int index1 = (index0 + 1) % bufferSize;
        float fraction = readPosition - static_cast<int>(readPosition);

		outputMonoSum = 0.0f;
        inputMonoSum = 0.0f;

		for (int channel = 0; channel < totalNumInputChannels; ++channel) {
			auto* channelData = buffer.getWritePointer(channel);
			auto* delayData = delayBuffer.getWritePointer(channel);

            float inputSample = channelData[sample];
			float delayedSample = delayData[index0] * (1 - fraction) + delayData[index1] * fraction;
            float outputSample = outputHPF[channel].processSample(delayedSample) * mix;

            // Feedback uses the pre-mix delayed sample so the Amount knob can
            // change wet/dry balance without changing loop stability.
            float feedbackSample = std::tanh(delayedSample * feedback);
            delayData[writePosition] = inputSample + feedbackSample;
			inputSample *= (1.0f - mix);

			outputMonoSum += outputSample;
            inputMonoSum += inputSample;

            channelData[sample] = outputSample + inputSample;
            
            // For testing the modulator wave
            //channelData[sample] = offset;
        }
		outputMonoSum /= static_cast<float>(totalNumInputChannels);
		outputBuffer.setSample(0, outputWritePos, outputMonoSum);
		outputWritePos++;
		if (outputWritePos >= fftSize) outputWritePos = 0;
		inputMonoSum /= static_cast<float>(totalNumInputChannels);
		inputBuffer.setSample(0, inputWritePos, inputMonoSum);
		inputWritePos++;
		if (inputWritePos >= fftSize) inputWritePos = 0;
        writePosition++;
        if (writePosition >= bufferSize) writePosition = 0;
    }
}

//==============================================================================
bool ErodeAudioProcessor::hasEditor() const
{
    return true; 
}

juce::AudioProcessorEditor* ErodeAudioProcessor::createEditor()
{
    return new ErodeAudioProcessorEditor (*this);
}

//==============================================================================
void ErodeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    auto xml = state.createXml();
    copyXmlToBinary(*xml, destData);
}

void ErodeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
	if (xml != nullptr && xml->hasTagName(apvts.state.getType())) {
        auto state = juce::ValueTree::fromXml(*xml);
        apvts.replaceState(state);
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ErodeAudioProcessor();
}
