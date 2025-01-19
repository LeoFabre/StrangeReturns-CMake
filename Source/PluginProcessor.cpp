#include "PluginProcessor.h"
#include "PluginEditor.h"

StrangeReturnsAudioProcessor::StrangeReturnsAudioProcessor(AudioProcessorValueTreeState::ParameterLayout layout)
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
    parameters { layout },
    vts(*this, nullptr, Identifier("Parameters"), std::move(layout))
{
    vts.state.addListener(this);
}

StrangeReturnsAudioProcessor::~StrangeReturnsAudioProcessor()
{
}

//==============================================================================
const String StrangeReturnsAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool StrangeReturnsAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool StrangeReturnsAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool StrangeReturnsAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

//==============================================================================
void StrangeReturnsAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    delayProcessor.prepareToPlay(sampleRate, samplesPerBlock);
}

void StrangeReturnsAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool StrangeReturnsAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
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

void StrangeReturnsAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    if (requiresUpdate.load())
    {
        auto timePot_ms = parameters.time.get();
        auto feedback = parameters.feedback.get();
        auto toneType = parameters.toneType.getIndex();

        auto modRate = parameters.modRate.get();
        auto modDepth = parameters.modDepth.get();
        auto modWave = parameters.modWave.getIndex();

        auto noiseLevel = parameters.noiseLevel.get();
        auto noiseType = parameters.noiseType.getIndex();

        delayProcessor.setDelayParameters(
            (parameters.tapTempoEnabled.get() ? TapTempoTime_ms : timePot_ms)
            , feedback, toneType, modRate, modDepth, modWave, noiseLevel, noiseType);

        auto effectsRouting = parameters.effectsRouting.getIndex();

        auto flipPhase = parameters.flipPhase.get();

        auto bcDepth = parameters.bcDepth.get();

        auto decimReduction = parameters.decimReduction.get();
        auto decimStereoSpread = parameters.decimStereoSpread.get();

        auto lpfCutoff = parameters.lpfCutoff.get();
        auto lpfQ = parameters.lpfQ.get();
        auto lpfPosition = parameters.lpfPosition.getIndex();

        auto bmLevel = parameters.bmLevel.get();
        auto bmOperation = parameters.bmOperation.getIndex();
        auto bmOperands = parameters.bmOperands.getIndex();

        delayProcessor.setEffectsParameters(effectsRouting, flipPhase, bcDepth, decimReduction, decimStereoSpread, lpfCutoff, lpfQ, lpfPosition, bmLevel, bmOperation, bmOperands);

        requiresUpdate.store(false);
    }

    delayProcessor.processBlock(buffer);
}

//==============================================================================
bool StrangeReturnsAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* StrangeReturnsAudioProcessor::createEditor()
{
    return new StrangeReturnsAudioProcessorEditor (*this);
}

//==============================================================================
void StrangeReturnsAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    copyXmlToBinary(*vts.copyState().createXml(), destData);
}

void StrangeReturnsAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    vts.replaceState(ValueTree::fromXml(*getXmlFromBinary(data, sizeInBytes)));
}

//==============================================================================
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StrangeReturnsAudioProcessor();
}

void StrangeReturnsAudioProcessor::handleTapTempo(bool isPressed)
{
    if (isPressed)
    {
        isButtonHeld = true;
        tapTempoHeld.store(true);
        startTimer(1000); // Démarrer le timer pour 1 seconde
    }
    else
    {
        if (tapTempoHeld.load())
        {
            // Si le bouton est relâché avant 1 seconde
            isButtonHeld = false;
            stopTimer();
            // Gérer le tap tempo normalement
            auto now = std::chrono::steady_clock::now();
            std::lock_guard<std::mutex> lock(tapMutex);

            // Ajoute le temps actuel aux taps
            tapTimes.push_back(now);

            // Supprime les taps plus vieux que 2 secondes
            while (!tapTimes.empty() && std::chrono::duration_cast<std::chrono::seconds>(now - tapTimes.front()).count() > 2)
            {
                tapTimes.pop_front();
            }

            // Si la durée entre le dernier tap et le premier est inférieure à 2 secondes et qu'il y a au moins 2 taps
            if (tapTimes.size() >= 2 && std::chrono::duration_cast<std::chrono::milliseconds>(tapTimes.back() - tapTimes.front()).count() <= 2000)
            {
                // Calcul de la moyenne des intervalles entre les taps
                std::deque<std::chrono::steady_clock::time_point> recentTaps = tapTimes;
                if (recentTaps.size() > 3)
                {
                    recentTaps.pop_front();
                }

                std::vector<float> intervals;
                for (size_t i = 1; i < recentTaps.size(); ++i)
                {
                    auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(recentTaps[i] - recentTaps[i - 1]).count();
                    intervals.push_back(static_cast<float>(interval));
                }

                if (!intervals.empty())
                {
                    float averageInterval = 0.0f;
                    for (auto interval : intervals)
                        averageInterval += interval;
                    averageInterval /= intervals.size();

                    // Mettre à jour la valeur de delay time
                    TapTempoTime_ms = averageInterval;
                    TimeAtTapTempoActivation = parameters.time.get();

                    // Activer le mode Tap Tempo
                    parameters.tapTempoEnabled.setValueNotifyingHost(true);
                    DBG("Tap tempo enabled. Time : " + String(TapTempoTime_ms) + "ms");

                    // Informer le DelayProcessor
                    delayProcessor.setTapTempoTime(TapTempoTime_ms);
                    delayProcessor.setReferencePotPosition(TimeAtTapTempoActivation);
                    delayProcessor.setTapTempoEnabled(true);
                    requiresUpdate.store(true);
                }
            }

            // Réinitialiser le bouton Tap Tempo pour permettre de futurs taps
            parameters.tapTempoButton.setValueNotifyingHost(false);
            tapTempoHeld.store(false);
        }
    }
}

void StrangeReturnsAudioProcessor::timerCallback()
{
    if (isButtonHeld)
    {
        // Si le bouton est maintenu pendant plus d'une seconde, désactiver Tap Tempo
        DBG("Tap Tempo disabled");
        parameters.tapTempoEnabled.setValueNotifyingHost(false);
        delayProcessor.setTapTempoEnabled(false);
        isButtonHeld = false;
        tapTempoHeld.store(false);
        stopTimer();
    }
}
