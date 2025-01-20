#include "PluginProcessor.h"
#include "PluginEditor.h"

StrangeReturnsAudioProcessorEditor::StrangeReturnsAudioProcessorEditor (StrangeReturnsAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    addAllAndMakeVisible(*this, basicControls, modAndNoiseControls, phaseBitCrusherDecimatorControls, filterControls, bitModControls);

    addAndMakeVisible(tapTempoButton);
    tapTempoBtnAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getVts(), "tapTempoButton", tapTempoButton);

    tapTempoButton.onStateChange = [this] {
        if (tapTempoButton.getState() == Button::ButtonState::buttonDown) {
            audioProcessor.handleTapTempo(true);
            DBG("Tap !");
        }
        else {
            audioProcessor.handleTapTempo(false);
        }
    };

    setSize(900, 850);
}

StrangeReturnsAudioProcessorEditor::~StrangeReturnsAudioProcessorEditor()
{
}

void StrangeReturnsAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
    g.setColour (Colours::white);
    g.setFont (15.0f);
}

void StrangeReturnsAudioProcessorEditor::resized()
{
    const int rowHeight = 150;
    const auto rowWidth = getWidth() * 0.9f;
    const auto rowMarginLeft = getWidth() * 0.025f;
    const int rowGap = 5;

    int y = 0;

    basicControls.setBounds(rowMarginLeft, y, rowWidth, rowHeight);
    tapTempoButton.setBounds(rowMarginLeft, y + rowHeight, rowWidth, 50);

    y += rowHeight + rowGap + 50; // Ajustement pour le bouton Tap Tempo
    modAndNoiseControls.setBounds(rowMarginLeft, y, rowWidth, rowHeight);

    y += rowHeight + rowGap;
    phaseBitCrusherDecimatorControls.setBounds(rowMarginLeft, y, rowWidth, rowHeight);

    y += rowHeight + rowGap;
    filterControls.setBounds(rowMarginLeft, y, rowWidth, rowHeight);

    y += rowHeight + rowGap;
    bitModControls.setBounds(rowMarginLeft, y, rowWidth, rowHeight);
}
