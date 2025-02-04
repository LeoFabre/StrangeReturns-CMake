#pragma once

#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_dsp/juce_dsp.h"
#include "BitModulation.h"
#include "Constants.h"
#include "DCBlocker.h"
#include "NoiseGenerator.h"
#include "ProcessorUtils.h"
#include "VASVFilter.h"

using namespace juce;

class DelayProcessor
{
public:
    DelayProcessor() {}
    ~DelayProcessor() {}

    enum ToneType
    {
        DIGITAL,
        TAPE
    };

    enum NoiseType
    {
        WHITE,
        BROWNIAN,
        PINK
    };

    enum EffectsRouting
    {
        IN,
        OUT
    };

    enum FilterPosition
    {
        PRE_BITMOD,
        POST_BITMOD
    };

    enum BitModOperands
    {
        POST_FX_POST_FX,
        PRE_FX_POST_FX,
        DRY_POST_FX
    };

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void processBlock(AudioBuffer<float>& buffer);

    void setDelayParameters(float time_ms, float feedback_pct, int _toneType, float _modRate_Hz, float modDepth_pct, int _modWave, float _noiseLevel_dB, int _noiseType)
    {
        time_smpls.setTargetValue(jmax(MIN_DELAY_SMPLS, time_ms * 0.001f * fs));
        feedback_lin.setTargetValue(feedback_pct * 0.01f);
        toneType = static_cast<ToneType>(_toneType);

        modRate_Hz.setTargetValue(jmax(MIN_MOD_RATE_HZ, _modRate_Hz));
        modDepth_lin.setTargetValue(modDepth_pct * 0.01f);
        modWave = static_cast<FastMathLFO::LFOWave>(_modWave);

        if (noiseLevel_dB != _noiseLevel_dB)
        {
            noiseLevel_dB = _noiseLevel_dB;
            noiseLevel_lin.setTargetValue(Decibels::decibelsToGain(noiseLevel_dB));
        }
        noiseType = static_cast<NoiseType>(_noiseType);
    }

    void setEffectsParameters(int _effectsRouting, bool _flipPhase, float _bcDepth_lin, float _decimReduction_lin,
                              float _decimStereoSpread_lin, float _lpfCutoff_Hz, float _lpfQ_lin, int _lpfPosition,
                              float _bmLevel_dB, int _bmOperation, int _bmOperands, float _hpfCutoff_Hz, float _hpfQ_lin, int _hpfPosition)
    {

        effectsRouting = static_cast<EffectsRouting>(_effectsRouting);

        smoothedPhaseFlip.setTargetValue(_flipPhase ? -1.0f : 1.0f);

        bcDepth_lin.setTargetValue(_bcDepth_lin);

        decimReduction_lin.setTargetValue(jmax(MIN_DECIMATOR_RATIO, _decimReduction_lin));
        decimStereoSpread_lin.setTargetValue(_decimStereoSpread_lin);

        lpfCutoff_Hz.setTargetValue(_lpfCutoff_Hz);
        lpfQ_lin.setTargetValue(_lpfQ_lin);
        lpfPosition = static_cast<FilterPosition>(_lpfPosition);

        hpfCutoff_Hz.setTargetValue(_hpfCutoff_Hz);
        hpfQ_lin.setTargetValue(_hpfQ_lin);
        hpfPosition = static_cast<FilterPosition>(_hpfPosition);

        if (bmLevel_dB != _bmLevel_dB) {
            bmLevel_dB = _bmLevel_dB;
            bmLevel_lin.setTargetValue(Decibels::decibelsToGain(bmLevel_dB));
        }
        bmOperation = static_cast<BitModulation::Operation>(_bmOperation);
        bitModOpFunc = BitModulation::getOpFunc(bmOperation);
        bmOperands = static_cast<BitModOperands>(_bmOperands);
    }

    // Tap Tempo
    void setTapTempoTime(float baseDelay_ms) { BaseDelayTime_ms = baseDelay_ms; }
    void setReferencePotPosition(float referencePosition) { ReferencePotPosition = referencePosition; }
    void setTapTempoEnabled(bool enabled) { TapTempoEnabled = enabled; }

private:
    float fs = 44100.0f;

    static constexpr float MIN_DELAY_SMPLS = 2.0f;
    static constexpr float MAX_MOD_DEPTH_SECS = 0.02f;
    static constexpr float TAPE_DEL_LOOP_GAIN = 3.98f;

    EffectsRouting effectsRouting = EffectsRouting::OUT;

    // delay
    SmoothedValM time_smpls = 1.0f;
    SmoothedValL feedback_lin = 0.0f;
    ToneType toneType = ToneType::DIGITAL;

    CircularBuffer delayBuffer[NUM_CHANNELS];
    StaticVASVFilter tapeDelayBandpass[NUM_CHANNELS];
    StaticVASVFilter delayHiPass[NUM_CHANNELS];

    // modulation
    float maxModDepth_smpls = MAX_MOD_DEPTH_SECS * 44100.0f;
    SmoothedValM modRate_Hz = MIN_MOD_RATE_HZ;
    SmoothedValL modDepth_lin = 0.0f;
    FastMathLFO::LFOWave modWave = FastMathLFO::LFOWave::TRI;

    FastMathLFO modLfo[NUM_CHANNELS];

    // noise
    SmoothedValM noiseLevel_lin = 0.001f;
    float noiseLevel_dB = MIN_GAIN_DB;
    NoiseType noiseType = NoiseType::WHITE;

    WhiteNoiseGenerator whiteNoiseGen;
    BrownianNoiseGenerator brownianNoiseGen;

    // phase
    SmoothedValL smoothedPhaseFlip = 1.0f;

    // bit crusher
    SmoothedValL bcDepth_lin = 0.0f;

    // decimator
    SmoothedValM decimReduction_lin = 1.0f;
    SmoothedValL decimStereoSpread_lin = 0.0f;
    float decimPhasor[NUM_CHANNELS] { 0.0f, 0.0f };
    float decimCurrentOutput[NUM_CHANNELS] { 0.0f, 0.0f };

    // low pass filter
    SmoothedValM lpfCutoff_Hz = MAX_FILTER_CUTOFF_FREQ;
    SmoothedValL lpfQ_lin = MIN_FILTER_Q;
    FilterPosition lpfPosition = FilterPosition::PRE_BITMOD;
    StaticVASVFilter lpf[NUM_CHANNELS];

    // high pass filter
    SmoothedValM hpfCutoff_Hz = MIN_FILTER_CUTOFF_FREQ;
    SmoothedValL hpfQ_lin = MIN_FILTER_Q;
    FilterPosition hpfPosition = FilterPosition::PRE_BITMOD;
    StaticVASVFilter hpf[NUM_CHANNELS];

    // bit modulation
    SmoothedValM bmLevel_lin = 0.01f;
    float bmLevel_dB = MIN_GAIN_DB;
    BitModulation::Operation bmOperation = BitModulation::Operation::NONE;
    BitModOperands bmOperands = BitModOperands::POST_FX_POST_FX;

    BitModulation::OperationFunc bitModOpFunc = BitModulation::getOpFunc(BitModulation::Operation::NONE);
    DCBlocker dcBlocker[NUM_CHANNELS];

    // Tap Tempo
    bool TapTempoEnabled = false;
    float BaseDelayTime_ms = 500.0f;
    float ReferencePotPosition = 0.25f;

    inline float applyBitCrusher(float x, float depth);
    inline float applyBitMod(float dry, float wet, float wetWithFx, float level);
    inline float applyDecimator(float x, float reduction, float stereoSpread, int channel);
    
    inline float applyEffects(float xDry, float xWet, float phaseFlipSmoothed, float bcDepth, float decimReduction, float decimStereoSpread, float bmLevel, int channel);
    
    inline float softClipper(float x)
    {
        return std::tanh(x);
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayProcessor)
};
