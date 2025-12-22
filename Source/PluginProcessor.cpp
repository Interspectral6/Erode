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
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "mode",
        "Mode",
        juce::StringArray{ "rough", "smooth" },
        0));
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
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
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
    float amount = apvts.getRawParameterValue("amount")->load() * 20;
    float lfoFreq = apvts.getRawParameterValue("freq")->load();
	bool isSmooth = apvts.getRawParameterValue("mode")->load() == 1;

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, numSamples);

    for (int sample = 0; sample < numSamples; ++sample) {
        offset = std::sin(lfoPhase);
        lfoPhase += twoPi * lfoFreq / sampleRate;
        if (lfoPhase >= twoPi) lfoPhase -= twoPi;

		float readPosition = writePosition - delayInSamples + offset * amount;
		if (readPosition < 0) readPosition += bufferSize;
		int index0 = static_cast<int>(readPosition) % bufferSize;
		int index1 = (index0 + 1) % bufferSize;
        float fraction = 0;

        if (isSmooth) fraction = readPosition - static_cast<int>(readPosition);

		for (int channel = 0; channel < totalNumInputChannels; ++channel) {
			auto* channelData = buffer.getWritePointer(channel);
			auto* delayData = delayBuffer.getWritePointer(channel);

            const float inputSample = channelData[sample];
			const float outputSample = delayData[index0] * (1 - fraction) + delayData[index1] * fraction;

            float mix = 0.5f;
            delayData[writePosition] = inputSample;
            channelData[sample] = outputSample * mix + inputSample * (1 - mix);
        }
        writePosition++;
        if (writePosition >= bufferSize) writePosition = 0;
    }
}

//==============================================================================
bool ErodeAudioProcessor::hasEditor() const
{
    return false; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ErodeAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

//==============================================================================
void ErodeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void ErodeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ErodeAudioProcessor();
}
