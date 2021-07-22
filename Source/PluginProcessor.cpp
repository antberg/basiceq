/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace BasicEQ;

using std::string;
using std::make_unique;
using std::move;

//==============================================================================
BasicEQAudioProcessor::BasicEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
	: AudioProcessor( BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
					  .withInput( "Input", juce::AudioChannelSet::stereo(), true )
#endif
					  .withOutput( "Output", juce::AudioChannelSet::stereo(), true )
#endif
	)
#endif
{}

BasicEQAudioProcessor::~BasicEQAudioProcessor()
{}

//==============================================================================
const juce::String BasicEQAudioProcessor::getName() const
{
	return JucePlugin_Name;
}

bool BasicEQAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
	return true;
#else
	return false;
#endif
}

bool BasicEQAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
	return true;
#else
	return false;
#endif
}

bool BasicEQAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
	return true;
#else
	return false;
#endif
}

double BasicEQAudioProcessor::getTailLengthSeconds() const
{
	return 0.0;
}

int BasicEQAudioProcessor::getNumPrograms()
{
	return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
				// so this should be at least 1, even if you're not really implementing programs.
}

int BasicEQAudioProcessor::getCurrentProgram()
{
	return 0;
}

void BasicEQAudioProcessor::setCurrentProgram( int index )
{}

const juce::String BasicEQAudioProcessor::getProgramName( int index )
{
	return {};
}

void BasicEQAudioProcessor::changeProgramName( int index, const juce::String& newName )
{}

//==============================================================================
void BasicEQAudioProcessor::prepareToPlay( double sampleRate, int samplesPerBlock )
{
	// Init process spec object.
	juce::dsp::ProcessSpec spec{ sampleRate, samplesPerBlock, 1 };

	// Prepare chains for processing.
	m_leftChain.prepare( spec );
	m_rightChain.prepare( spec );
}

void BasicEQAudioProcessor::releaseResources()
{
	// When playback stops, you can use this as an opportunity to free up any
	// spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool BasicEQAudioProcessor::isBusesLayoutSupported( const BusesLayout& layouts ) const
{
#if JucePlugin_IsMidiEffect
	juce::ignoreUnused( layouts );
	return true;
#else
	// This is the place where you check if the layout is supported.
	// In this template code we only support mono or stereo.
	// Some plugin hosts, such as certain GarageBand versions, will only
	// load plugins that support stereo bus layouts.
	if ( layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
		 && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo() )
		return false;

	// This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
	if ( layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet() )
		return false;
#endif

	return true;
#endif
}
#endif

void BasicEQAudioProcessor::processBlock( juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages )
{
	juce::ScopedNoDenormals noDenormals;
	auto totalNumInputChannels = getTotalNumInputChannels();
	auto totalNumOutputChannels = getTotalNumOutputChannels();

	// In case we have more outputs than inputs, this code clears any output
	// channels that didn't contain input data, (because these aren't
	// guaranteed to be empty - they may contain garbage).
	// This is here to avoid people getting screaming feedback
	// when they first compile a plugin, but obviously you don't need to keep
	// this code if your algorithm always overwrites all the output channels.
	for ( auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i )
		buffer.clear( i, 0, buffer.getNumSamples() );

	// Create audio block.
	juce::dsp::AudioBlock<float> block{ buffer };
	auto leftBlock = block.getSingleChannelBlock( 0 );
	auto rightBlock = block.getSingleChannelBlock( 1 );

	// Create processing contexts.
	juce::dsp::ProcessContextReplacing<float> leftContext{ leftBlock };
	juce::dsp::ProcessContextReplacing<float> rightContext{ rightBlock };

	// Do processing.
	m_leftChain.process( leftContext );
	m_rightChain.process( rightContext );
}

//==============================================================================
bool BasicEQAudioProcessor::hasEditor() const
{
	return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* BasicEQAudioProcessor::createEditor()
{
	//return new BasicEQAudioProcessorEditor( *this );
	return new juce::GenericAudioProcessorEditor( *this );
}

//==============================================================================
void BasicEQAudioProcessor::getStateInformation( juce::MemoryBlock& destData )
{
	// You should use this method to store your parameters in the memory block.
	// You could do that either as raw data, or use the XML or ValueTree classes
	// as intermediaries to make it easy to save and load complex data.
}

void BasicEQAudioProcessor::setStateInformation( const void* data, int sizeInBytes )
{
	// You should use this method to restore your parameters from this memory block,
	// whose contents will have been created by the getStateInformation() call.
}

juce::AudioProcessorValueTreeState::ParameterLayout BasicEQAudioProcessor::createParameterLayout()
{
	using juce::String;
	using juce::StringArray;
	using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;
	using Range = juce::NormalisableRange<float>;
	using Float = juce::AudioParameterFloat;
	using Choice = juce::AudioParameterChoice;

	// Initialize layout
	Layout layout{};

	// Initialize ranges
	Range freqRange{ 20.f,  20000.f, 1.f, 1.f }; // Hz
	Range gainRange{ -24.f, 24.f, 0.5f, 1.f }; // dB
	Range qualityRange{ 0.1f, 10.f, 0.05f, 1.f };

	// Initialize low-cut frequency parameter
	String lowcutName{ "Low-cut freq" };
	float lowcutDefault = 20.f;
	auto lowcutParameter = make_unique<Float>( lowcutName,
											   lowcutName,
											   freqRange,
											   lowcutDefault );

	// Initialize high-cut frequency parameter
	String highcutName{ "High-cut freq" };
	float highcutDefault = 20000.f;
	auto highcutParameter = make_unique<Float>( highcutName,
												highcutName,
												freqRange,
												highcutDefault );

	// Initialize peak frequency parameter
	String peakName{ "Peak freq" };
	float peakDefault = 750.f;
	auto peakParameter = make_unique<Float>( peakName,
											 peakName,
											 freqRange,
											 peakDefault );

	// Initialize peak gain parameter
	String gainName{ "Peak gain" };
	float gainDefault = 0.f;
	auto gainParameter = make_unique<Float>( gainName,
											 gainName,
											 gainRange,
											 gainDefault );

	// Initialize peak quality parameter
	String qualityName{ "Peak quality" };
	float qualityDefault = 1.f;
	auto qualityParameter = make_unique<Float>( qualityName,
												qualityName,
												gainRange,
												qualityDefault );

	// Create low-cut and high-cut slope options
	StringArray slopeOptions{};
	int slopeOptionCount{ 4 };
	int slopeDefaultIndex = 0;

	for ( int i = 0; i < slopeOptionCount; ++i )
	{
		String option{};
		option << ( i + 1 ) * 12 << " dB/oct";
		slopeOptions.add( option );
	}

	// Initialize low-cut slope parameter
	String lowcutSlopeName{ "Low-cut slope" };
	auto lowcutSlopeParameter = make_unique<Choice>( lowcutSlopeName,
													 lowcutSlopeName,
													 slopeOptions,
													 slopeDefaultIndex );

	// Initialize high-cut slope parameter
	String highcutSlopeName{ "High-cut slope" };
	auto highcutSlopeParameter = make_unique<Choice>( highcutSlopeName,
													  highcutSlopeName,
													  slopeOptions,
													  slopeDefaultIndex );

	// Add audio parameters to layout
	layout.add( move( lowcutParameter ) );
	layout.add( move( highcutParameter ) );
	layout.add( move( peakParameter ) );
	layout.add( move( gainParameter ) );
	layout.add( move( qualityParameter ) );
	layout.add( move( lowcutSlopeParameter ) );
	layout.add( move( highcutSlopeParameter ) );

	return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new BasicEQAudioProcessor();
}
