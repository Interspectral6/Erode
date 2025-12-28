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
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "freq",
        "Freq",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f),
        1000.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "width",
        "Width",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "amount",
        "Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.5f));
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
}

const juce::String ErodeAudioProcessor::getProgramName (int index)
{
    return {};
}

void ErodeAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ErodeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	const int numSamples = static_cast<int>(sampleRate * 0.1); // 100 ms max delay
    delayBuffer.setSize(getTotalNumOutputChannels(), numSamples);
    delayBuffer.clear();
	
    writePosition = 0;
    lfoPhase = 0.0f;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    filter.prepare(spec);
    filter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    outputHPF.reset();
	outputHPF.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 2000.0f, 0.5f);
    
	outputBuffer.setSize(1, fftSize);
    outputBuffer.clear();
    outputWritePos = 0;
	inputBuffer.setSize(1, fftSize);
    inputBuffer.clear();
	inputWritePos = 0;
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
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    const int numSamples = buffer.getNumSamples();
    const int bufferSize = delayBuffer.getNumSamples();
    const int delayInSamples = static_cast<int>(getSampleRate() * 0.05f);
    const float sampleRate = getSampleRate();
    float offset = 0.0f;
    float noise = 0.0f;
    float sine = 0.0f;
    float mix = apvts.getRawParameterValue("amount")->load();
    float amount = mix * 20.0f;
    float freq = apvts.getRawParameterValue("freq")->load();
    float width = apvts.getRawParameterValue("width")->load();
    float minQ = 0.5f;
    float maxQ = 30.0f;
    float q = minQ * std::pow(maxQ / minQ, 1.0f - width);
    float sineAmount = 1.0f - std::pow(width, 0.7f); // Lower coefficient means less sine
    float noiseAmount = 1.0f - sineAmount;
	float hpfFreq = apvts.getRawParameterValue("cut")->load();
	outputHPF.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, hpfFreq, 0.7f);

	filter.setCutoffFrequency(freq);
	filter.setResonance(q);

    float outputMonoSum = 0.0f;
	float inputMonoSum = 0.0f;

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, numSamples);

    for (int sample = 0; sample < numSamples; ++sample) {
		noise = rand.nextFloat() * 2.0f - 1.0f;
        
        // std::pow here is to balance the loudness of noise, since higher q means louder
		noise = filter.processSample(0, noise) * std::pow(width, 0.2f); // Lower coefficient means more noise
		noise = std::tanh(noise);

		sine = std::sin(lfoPhase);
		lfoPhase += twoPi * freq / sampleRate;
		if (lfoPhase >= twoPi) lfoPhase -= twoPi;

        // Crossfade between noise and sine
		offset = noiseAmount * noise + sineAmount * sine;

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
			float outputSample = delayData[index0] * (1 - fraction) + delayData[index1] * fraction;
            outputSample = outputHPF.processSample(outputSample) * mix;
            delayData[writePosition] = inputSample;
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
