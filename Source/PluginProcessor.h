#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <deque>
#include <chrono>
#include <mutex>

#include "Constants.h"
#include "DelayProcessor.h"

using namespace juce;

namespace paramID
{
#define PARAMETER_ID(str) constexpr const char* str { #str };

    // BASICS
    PARAMETER_ID(time)
    PARAMETER_ID(feedback)
    PARAMETER_ID(toneType)
    PARAMETER_ID(effectsRouting)
    PARAMETER_ID(tapTempoEnabled)
    PARAMETER_ID(tapTempoButton)
    PARAMETER_ID(beatMultiply)

    // MODULATION
    PARAMETER_ID(modRate)
    PARAMETER_ID(modDepth)
    PARAMETER_ID(modWave)

    // NOISE
    PARAMETER_ID(noiseLevel)
    PARAMETER_ID(noiseType)

    // PHASE
    PARAMETER_ID(flipPhase)

    // BIT CRUSHER
    PARAMETER_ID(bcDepth)

    // DECIMATOR
    PARAMETER_ID(decimReduction)
    PARAMETER_ID(decimStereoSpread)

    // LPF
    PARAMETER_ID(lpfCutoff)
    PARAMETER_ID(lpfQ)
    PARAMETER_ID(lpfPosition)

    // HPF
    PARAMETER_ID(hpfCutoff)
    PARAMETER_ID(hpfQ)
    PARAMETER_ID(hpfPosition)

    // BIT MODULATION
    PARAMETER_ID(bmLevel)
    PARAMETER_ID(bmOperation)
    PARAMETER_ID(bmOperands)

#undef PARAMETER_ID
}

template <typename Func, typename... Items>
constexpr void forEach(Func&& func, Items&&... items)
noexcept (noexcept (std::initializer_list<int> { (func(std::forward<Items>(items)), 0)... }))
{
    (void)std::initializer_list<int> { ((void)func(std::forward<Items>(items)), 0)... };
}

class StrangeReturnsAudioProcessor : public juce::AudioProcessor,
                                     private ValueTree::Listener,
                                     private Timer
{
public:
    //==============================================================================
    explicit StrangeReturnsAudioProcessor(AudioProcessorValueTreeState::ParameterLayout layout);
    StrangeReturnsAudioProcessor() : StrangeReturnsAudioProcessor(AudioProcessorValueTreeState::ParameterLayout{}) {};
    ~StrangeReturnsAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override { return 0.0; };

    //==============================================================================
    int getNumPrograms() override { return 1; };
    int getCurrentProgram() override { return 0; };
    void setCurrentProgram(int index) override {};
    const String getProgramName(int index) override { return {}; };
    void changeProgramName(int index, const String& newName) override {};

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    using Parameter = AudioProcessorValueTreeState::Parameter;
    struct ParameterReferences
    {
        template <typename Param>
        static Param& addToLayout(AudioProcessorValueTreeState::ParameterLayout& layout, std::unique_ptr<Param> param)
        {
            auto& ref = *param;
            layout.add(std::move(param));
            return ref;
        }

        static String valueToTextFunction(float x) { return String(x, 2); }
        static float textToValueFunction(const String& str) { return str.getFloatValue(); }

        static String bitCrushValueToTextFunction(float x)
        {
            return x >= MIN_BITCRUSHER_Q ? String(std::log2f((2.0f / x) + 1.0f), 2) : String("-");
        }

        static String decimReductionValueToTextFunction(float x)
        {
            return String(x, 5);
        }

        // static String frequencyValueToTextFunction(float x)
        // {
        //     if (x >= 1000.0f)
        //         x *= 0.001f;
        //
        //     return String(x, 2);
        // }

        static const StringArray beatMultiplyOptions()
        {
            return StringArray{
                "0.25",
                "0.3333333",
                "0.5",
                "0.6666666",
                "0.75",
                "1.0",
                "1.25",
                "1.333333",
                "1.5"
            };
        }

        static const StringArray toneTypeOptions() { return StringArray{ "DIGITAL", "TAPE" }; }

        static const StringArray effectsRoutingOptions() { return StringArray{ "IN", "OUT" }; }

        static const StringArray modWaveOptions() { return StringArray{ "SIN", "TRI" }; }

        static const StringArray noiseTypeOptions() { return StringArray{ "WHITE", "BROWNIAN", "PINK" }; }

        static const StringArray filterPositionOptions() { return StringArray{ "PRE BITMOD", "POST BITMOD" }; }

        static const StringArray bmOperationOptions() { return StringArray{ "NONE", "XOR", "AND", "OR" }; }

        static const StringArray bmOperandsOptions() { return StringArray{ "POST FX + POST FX", "PRE FX + POST FX", "DRY + POST FX" }; }

        explicit ParameterReferences(AudioProcessorValueTreeState::ParameterLayout& layout)
            : time(addToLayout(layout, std::make_unique<Parameter>(paramID::time, "Time", "ms", NormalisableRange<float>(50.0f, MAX_DELAY_TIME_SEC * 1000.0f, 1.0f, 0.5f), 100.0f, valueToTextFunction, textToValueFunction))),
              feedback(addToLayout(layout, std::make_unique<Parameter>(paramID::feedback, "Feedback", "%", NormalisableRange<float>(0.0f, 100.0f), 0.0f, valueToTextFunction, textToValueFunction))),
              toneType(addToLayout(layout, std::make_unique<AudioParameterChoice>(paramID::toneType, "Type", toneTypeOptions(), 0))),
              effectsRouting(addToLayout(layout, std::make_unique<AudioParameterChoice>(paramID::effectsRouting, "Effects Routing", effectsRoutingOptions(), 1))),
              tapTempoEnabled(addToLayout(layout, std::make_unique<AudioParameterBool>(paramID::tapTempoEnabled, "Tap Tempo Enabled", false))),
              tapTempoButton(addToLayout(layout, std::make_unique<AudioParameterBool>(paramID::tapTempoButton, "Tap Tempo Button", false))),
              beatMultiply(addToLayout(layout, std::make_unique<AudioParameterChoice>(paramID::beatMultiply, "Beat Multiply", beatMultiplyOptions(), 6))),
              modRate(addToLayout(layout, std::make_unique<Parameter>(paramID::modRate, "Mod Rate", "Hz", NormalisableRange<float>(MIN_MOD_RATE_HZ, MAX_MOD_RATE_HZ), MIN_MOD_RATE_HZ, valueToTextFunction, textToValueFunction))),
              modDepth(addToLayout(layout, std::make_unique<Parameter>(paramID::modDepth, "Mod Depth", "%", NormalisableRange<float>(0.0f, 100.0f), 0.0f, valueToTextFunction, textToValueFunction))),
              modWave(addToLayout(layout, std::make_unique<AudioParameterChoice>(paramID::modWave, "Mod Wave", modWaveOptions(), 1))),

              noiseLevel(addToLayout(layout, std::make_unique<Parameter>(paramID::noiseLevel, "Noise Level", "dB", NormalisableRange<float>(MIN_NOISE_LEVEL_DB, MAX_NOISE_LEVEL_DB), MIN_NOISE_LEVEL_DB, valueToTextFunction, textToValueFunction))),
              noiseType(addToLayout(layout, std::make_unique<AudioParameterChoice>(paramID::noiseType, "Noise Type", noiseTypeOptions(), 0))),

              flipPhase(addToLayout(layout, std::make_unique<AudioParameterBool>(paramID::flipPhase, "Flip Phase", false, ""))),

              bcDepth(addToLayout(layout, std::make_unique<Parameter>(paramID::bcDepth, "Bit Depth", "bits", NormalisableRange<float>(0.0f, MAX_BITCRUSHER_Q, 0.0f, 0.15f), MIN_BITCRUSHER_Q, bitCrushValueToTextFunction, textToValueFunction))),

              decimReduction(addToLayout(layout, std::make_unique<Parameter>(paramID::decimReduction, "Sample Rate Reduction", "", NormalisableRange<float>(MIN_DECIMATOR_RATIO, MAX_DECIMATOR_RATIO, 0.0f, 0.25f), MAX_DECIMATOR_RATIO, decimReductionValueToTextFunction, textToValueFunction))),
              decimStereoSpread(addToLayout(layout, std::make_unique<Parameter>(paramID::decimStereoSpread, "Stereo Spread", "", NormalisableRange<float>(0.0f, 0.5f), 0.0f, valueToTextFunction, textToValueFunction))),

              hpfCutoff(addToLayout(layout, std::make_unique<Parameter>(paramID::hpfCutoff, "LowCut Cutoff", "Hz", NormalisableRange<float> (MIN_FILTER_CUTOFF_FREQ, MAX_FILTER_CUTOFF_FREQ, 0.0f, 0.25f), MIN_FILTER_CUTOFF_FREQ, valueToTextFunction, textToValueFunction))),
              hpfQ(addToLayout(layout, std::make_unique<Parameter>(paramID::hpfQ, "LowCut Resonance", "", NormalisableRange<float>(MIN_FILTER_Q, MAX_FILTER_Q), MIN_FILTER_Q, valueToTextFunction, textToValueFunction))),
              hpfPosition(addToLayout(layout, std::make_unique<AudioParameterChoice>(paramID::hpfPosition, "LowCut Position", filterPositionOptions(), 0))),

              lpfCutoff(addToLayout(layout, std::make_unique<Parameter>(paramID::lpfCutoff, "HighCut Cutoff", "Hz", NormalisableRange<float> (MIN_FILTER_CUTOFF_FREQ, MAX_FILTER_CUTOFF_FREQ, 0.0f, 0.25f), MAX_FILTER_CUTOFF_FREQ, valueToTextFunction, textToValueFunction))),
              lpfQ(addToLayout(layout, std::make_unique<Parameter>(paramID::lpfQ, "HighCut Resonance", "", NormalisableRange<float>(MIN_FILTER_Q, MAX_FILTER_Q), MIN_FILTER_Q, valueToTextFunction, textToValueFunction))),
              lpfPosition(addToLayout(layout, std::make_unique<AudioParameterChoice>(paramID::lpfPosition, "HighCut Position", filterPositionOptions(), 0))),

              bmLevel(addToLayout(layout, std::make_unique<Parameter>(paramID::bmLevel, "BitMod Level", "dB", NormalisableRange<float> (MIN_GAIN_DB, MAX_GAIN_DB), MIN_GAIN_DB, valueToTextFunction, textToValueFunction))),
              bmOperation(addToLayout(layout, std::make_unique<AudioParameterChoice>(paramID::bmOperation, "BitMod Operation", bmOperationOptions(), 0))),
              bmOperands(addToLayout(layout, std::make_unique<AudioParameterChoice>(paramID::bmOperands, "BitMod Operands", bmOperandsOptions(), 0)))
        {}

        Parameter& time;
        Parameter& feedback;
        AudioParameterChoice& beatMultiply;
        AudioParameterChoice& toneType;
        AudioParameterChoice& effectsRouting;
        AudioParameterBool& tapTempoEnabled;
        AudioParameterBool& tapTempoButton;

        Parameter& modRate;
        Parameter& modDepth;
        AudioParameterChoice& modWave;

        Parameter& noiseLevel;
        AudioParameterChoice& noiseType;

        AudioParameterBool& flipPhase;

        Parameter& bcDepth;

        Parameter& decimReduction;
        Parameter& decimStereoSpread;

        Parameter& hpfCutoff;
        Parameter& hpfQ;
        AudioParameterChoice& hpfPosition;

        Parameter& lpfCutoff;
        Parameter& lpfQ;
        AudioParameterChoice& lpfPosition;

        Parameter& bmLevel;
        AudioParameterChoice& bmOperation;
        AudioParameterChoice& bmOperands;
    };

    const ParameterReferences& getParameterValues() const noexcept { return parameters; }
    AudioProcessorValueTreeState& getVts() { return vts; }

    void handleTapTempo(bool isPressed);

private:
    void valueTreePropertyChanged(ValueTree&, const Identifier&) override
    {
        requiresUpdate.store(true);
    }

    // Tap Tempo
    std::deque<std::chrono::steady_clock::time_point> tapTimes;
    std::mutex tapMutex;
    bool TapTempoEnabled = false;
    float TapTempoTime_ms = -1.0f;
    float TimeAtTapTempoActivation = -1.0f;
    bool isButtonHeld = false;
    std::atomic<bool> tapTempoHeld{ false };
    void timerCallback() override;

    ParameterReferences parameters;
    AudioProcessorValueTreeState vts;
    std::atomic<bool> requiresUpdate{ true };

    DelayProcessor delayProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StrangeReturnsAudioProcessor)
};
