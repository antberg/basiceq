#pragma once

#include <JuceHeader.h>

namespace BasicEQ
{

class BasicEQAudioProcessor : public juce::AudioProcessor
{
	/**
	 * Peak-filter type.
	 */
	using Filter = juce::dsp::IIR::Filter<float>;

	/**
	 * Cut-filter type (Low-pass and high-pass filters have a response of 12 dB/oct).
	 */
	using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

	/**
	 * Chain of the three basic EQ filters (low-cut, parametric, high-cut).
	 */
	using MonoChain = juce::dsp::ProcessorChain<Filter, CutFilter>;

	/**
	 * Audio processor value tree state.
	 */
	using APVTS = juce::AudioProcessorValueTreeState;

public:
	BasicEQAudioProcessor();
	~BasicEQAudioProcessor() override;

	void prepareToPlay( double sampleRate, int samplesPerBlock ) override;
	void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
	bool isBusesLayoutSupported( const BusesLayout& layouts ) const override;
#endif

	void processBlock( juce::AudioBuffer<float>&, juce::MidiBuffer& ) override;

	juce::AudioProcessorEditor* createEditor() override;
	bool hasEditor() const override;

	const juce::String getName() const override;

	bool acceptsMidi() const override;
	bool producesMidi() const override;
	bool isMidiEffect() const override;
	double getTailLengthSeconds() const override;

	int getNumPrograms() override;
	int getCurrentProgram() override;
	void setCurrentProgram( int index ) override;
	const juce::String getProgramName( int index ) override;
	void changeProgramName( int index, const juce::String& newName ) override;

	void getStateInformation( juce::MemoryBlock& destData ) override;
	void setStateInformation( const void* data, int sizeInBytes ) override;

	/**
	 * Create parameter layout.
	 */
	static APVTS::ParameterLayout createParameterLayout();

private:
	/**
	 * Stereo processor chains.
	 */
	MonoChain m_leftChain, m_rightChain;

	/**
	 * Audio processor VTS.
	 */
	APVTS m_audioProcessorVTS{ *this, nullptr, "Parameters", createParameterLayout() };

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR( BasicEQAudioProcessor )
};

}