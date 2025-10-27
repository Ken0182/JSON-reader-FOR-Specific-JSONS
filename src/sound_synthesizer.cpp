/**
 * @file sound_synthesizer.cpp
 * @brief Real-time Sound Synthesizer Engine - Implementation
 * @author AI Assistant
 * @version 1.0
 */

#include "sound_synthesizer.hpp"
#include <iostream>
#include <algorithm>
#include <cstring>
#include <alsa/asoundlib.h>  // ALSA for audio output
#include <alsa/seq.h>        // ALSA MIDI
#include <alsa/seq_midi_event.h>

namespace audio_config {

// ============================================================================
// Voice Implementation
// ============================================================================

void Voice::startNote(const MIDINote& note, const SynthesisParams& params) {
    isActive_ = true;
    noteNumber_ = note.noteNumber;
    frequency_ = note.frequency;
    velocity_ = note.velocity;
    phase_ = 0.0;
    envelopePhase_ = 0.0;
    envelopeValue_ = 0.0;
    envelopeState_ = EnvelopeState::Attack;
    
    // Calculate envelope rates
    envelopeTarget_ = 1.0;
    envelopeRate_ = 1.0 / (params.attack * SAMPLE_RATE);
}

void Voice::stopNote() {
    if (isActive_ && envelopeState_ != EnvelopeState::Release) {
        envelopeState_ = EnvelopeState::Release;
        envelopeTarget_ = 0.0;
        // Use a default release time if no params available
        envelopeRate_ = 1.0 / (0.5 * SAMPLE_RATE); // 0.5 second default release
    }
}

void Voice::update(double deltaTime, const SynthesisParams& params) {
    if (!isActive_) return;
    
    updateEnvelope(deltaTime, params);
    
    // Check if envelope is complete
    if (envelopeState_ == EnvelopeState::Off) {
        isActive_ = false;
    }
}

double Voice::generateSample(const SynthesisParams& params) {
    if (!isActive_) return 0.0;
    
    double sample = generateOscillator(params);
    sample = applyFilter(sample, params);
    sample = applyEffects(sample, params);
    
    // Apply envelope
    sample *= envelopeValue_ * velocity_;
    
    // Update phase
    phase_ += frequency_ / SAMPLE_RATE;
    if (phase_ >= 1.0) phase_ -= 1.0;
    
    return sample;
}

double Voice::generateOscillator(const SynthesisParams& params) {
    if (params.oscillatorTypes.empty()) return 0.0;
    
    double sample = 0.0;
    double phase = phase_;
    
    for (size_t i = 0; i < params.oscillatorTypes.size() && i < params.oscillatorMix.size(); ++i) {
        double mix = params.oscillatorMix[i];
        double oscSample = 0.0;
        
        if (params.oscillatorTypes[i] == "sine") {
            oscSample = std::sin(2.0 * M_PI * phase);
        } else if (params.oscillatorTypes[i] == "sawtooth") {
            oscSample = 2.0 * phase - 1.0;
        } else if (params.oscillatorTypes[i] == "square") {
            oscSample = (phase < 0.5) ? 1.0 : -1.0;
        } else if (params.oscillatorTypes[i] == "triangle") {
            if (phase < 0.5) {
                oscSample = 4.0 * phase - 1.0;
            } else {
                oscSample = 3.0 - 4.0 * phase;
            }
        } else if (params.oscillatorTypes[i] == "pulse") {
            double pulseWidth = 0.5; // Default pulse width
            oscSample = (phase < pulseWidth) ? 1.0 : -1.0;
        }
        
        sample += oscSample * mix;
    }
    
    return sample;
}

double Voice::applyFilter(double sample, const SynthesisParams& params) {
    // Simple one-pole low-pass filter
    if (params.filterType == "lowpass") {
        static double lastOutput = 0.0;
        double cutoff = params.cutoff / SAMPLE_RATE;
        double alpha = std::exp(-2.0 * M_PI * cutoff);
        lastOutput = alpha * lastOutput + (1.0 - alpha) * sample;
        return lastOutput;
    }
    
    return sample; // No filtering
}

double Voice::applyEffects(double sample, const SynthesisParams& params) {
    // Simple distortion
    if (params.distortionAmount > 0.0) {
        sample = std::tanh(sample * (1.0 + params.distortionAmount * 10.0));
    }
    
    return sample;
}

void Voice::updateEnvelope(double /* deltaTime */, const SynthesisParams& params) {
    switch (envelopeState_) {
        case EnvelopeState::Attack:
            envelopeValue_ += envelopeRate_;
            if (envelopeValue_ >= envelopeTarget_) {
                envelopeValue_ = envelopeTarget_;
                envelopeState_ = EnvelopeState::Decay;
                envelopeTarget_ = params.sustain;
                envelopeRate_ = (envelopeTarget_ - envelopeValue_) / (params.decay * SAMPLE_RATE);
            }
            break;
            
        case EnvelopeState::Decay:
            envelopeValue_ += envelopeRate_;
            if (envelopeValue_ <= envelopeTarget_) {
                envelopeValue_ = envelopeTarget_;
                envelopeState_ = EnvelopeState::Sustain;
            }
            break;
            
        case EnvelopeState::Sustain:
            // Sustain level - no change
            break;
            
        case EnvelopeState::Release:
            envelopeValue_ += envelopeRate_;
            if (envelopeValue_ <= 0.0) {
                envelopeValue_ = 0.0;
                envelopeState_ = EnvelopeState::Off;
            }
            break;
            
        case EnvelopeState::Off:
            // Envelope is off
            break;
    }
}

// ============================================================================
// SoundSynthesizer Implementation
// ============================================================================

SoundSynthesizer::SoundSynthesizer() 
    : isRunning_(false), globalVolume_(0.8), shouldStop_(false), audioDevice_(nullptr) {
}

SoundSynthesizer::~SoundSynthesizer() {
    stopPlayback();
    cleanupAudioOutput();
}

bool SoundSynthesizer::initialize() {
    if (!initializeAudioOutput()) {
        std::cerr << "Failed to initialize audio output" << std::endl;
        return false;
    }
    
    return true;
}

bool SoundSynthesizer::loadConfiguration(const AudioConfig& config) {
    currentParams_ = parseConfiguration(config);
    return true;
}

bool SoundSynthesizer::startPlayback() {
    if (isRunning_) return true;
    
    shouldStop_ = false;
    isRunning_ = true;
    
    // Start audio thread
    audioThread_ = std::thread(&SoundSynthesizer::processAudio, this);
    
    return true;
}

void SoundSynthesizer::stopPlayback() {
    if (!isRunning_) return;
    
    shouldStop_ = true;
    isRunning_ = false;
    
    if (audioThread_.joinable()) {
        audioThread_.join();
    }
    
    allNotesOff();
}

void SoundSynthesizer::noteOn(int noteNumber, double velocity) {
    std::lock_guard<std::mutex> lock(audioMutex_);
    
    int voiceIndex = findFreeVoice();
    if (voiceIndex >= 0) {
        MIDINote note(noteNumber, velocity, 0.0); // Time will be set by audio thread
        voices_[voiceIndex].startNote(note, currentParams_);
    }
}

void SoundSynthesizer::noteOff(int noteNumber) {
    std::lock_guard<std::mutex> lock(audioMutex_);
    
    Voice* voice = findVoiceByNote(noteNumber);
    if (voice) {
        voice->stopNote();
    }
}

void SoundSynthesizer::allNotesOff() {
    std::lock_guard<std::mutex> lock(audioMutex_);
    
    for (auto& voice : voices_) {
        voice.stopNote();
    }
}

void SoundSynthesizer::setVolume(double volume) {
    globalVolume_ = std::clamp(volume, 0.0, 1.0);
}

void SoundSynthesizer::updateParameters(const SynthesisParams& params) {
    std::lock_guard<std::mutex> lock(audioMutex_);
    currentParams_ = params;
}

void SoundSynthesizer::audioCallback(float* outputBuffer, int numFrames) {
    std::lock_guard<std::mutex> lock(audioMutex_);
    
    double deltaTime = numFrames / SAMPLE_RATE;
    
    // Clear output buffer
    std::memset(outputBuffer, 0, numFrames * 2 * sizeof(float)); // Stereo
    
    // Process each voice
    for (auto& voice : voices_) {
        if (!voice.isActive()) continue;
        
        voice.update(deltaTime, currentParams_);
        
        for (int i = 0; i < numFrames; ++i) {
            double sample = voice.generateSample(currentParams_);
            sample *= globalVolume_ * currentParams_.volume;
            
            // Apply panning (simple stereo)
            double leftGain = (1.0 - currentParams_.pan) * 0.5;
            double rightGain = (1.0 + currentParams_.pan) * 0.5;
            
            outputBuffer[i * 2] += sample * leftGain;     // Left channel
            outputBuffer[i * 2 + 1] += sample * rightGain; // Right channel
        }
    }
}

void SoundSynthesizer::processAudio() {
    // Simple audio processing loop
    // In a real implementation, this would use ALSA callbacks
    const int bufferSize = BUFFER_SIZE;
    std::vector<float> buffer(bufferSize * 2); // Stereo
    
    while (!shouldStop_) {
        audioCallback(buffer.data(), bufferSize);
        
        // In a real implementation, this would write to ALSA
        // For now, we'll just sleep to prevent busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

SynthesisParams SoundSynthesizer::parseConfiguration(const AudioConfig& config) {
    SynthesisParams params;
    
    const auto& configData = config.getConfigData();
    
    // Parse ADSR
    if (configData.contains("adsr")) {
        parseADSR(configData["adsr"], params);
    }
    
    // Parse oscillator
    if (configData.contains("oscillator")) {
        parseOscillator(configData["oscillator"], params);
    }
    
    // Parse filter
    if (configData.contains("filter")) {
        parseFilter(configData["filter"], params);
    }
    
    // Parse effects
    if (configData.contains("effects")) {
        parseEffects(configData["effects"], params);
    }
    
    return params;
}

void SoundSynthesizer::parseADSR(const nlohmann::json& adsr, SynthesisParams& params) {
    if (adsr.contains("attack")) {
        if (adsr["attack"].is_array() && adsr["attack"].size() >= 2) {
            params.attack = (adsr["attack"][0].get<double>() + adsr["attack"][1].get<double>()) / 2.0 / 1000.0; // Convert ms to seconds
        } else if (adsr["attack"].is_number()) {
            params.attack = adsr["attack"].get<double>() / 1000.0;
        }
    }
    
    if (adsr.contains("decay")) {
        if (adsr["decay"].is_array() && adsr["decay"].size() >= 2) {
            params.decay = (adsr["decay"][0].get<double>() + adsr["decay"][1].get<double>()) / 2.0 / 1000.0;
        } else if (adsr["decay"].is_number()) {
            params.decay = adsr["decay"].get<double>() / 1000.0;
        }
    }
    
    if (adsr.contains("sustain")) {
        if (adsr["sustain"].is_array() && adsr["sustain"].size() >= 2) {
            params.sustain = (adsr["sustain"][0].get<double>() + adsr["sustain"][1].get<double>()) / 2.0;
        } else if (adsr["sustain"].is_number()) {
            params.sustain = adsr["sustain"].get<double>();
        }
    }
    
    if (adsr.contains("release")) {
        if (adsr["release"].is_array() && adsr["release"].size() >= 2) {
            params.release = (adsr["release"][0].get<double>() + adsr["release"][1].get<double>()) / 2.0 / 1000.0;
        } else if (adsr["release"].is_number()) {
            params.release = adsr["release"].get<double>() / 1000.0;
        }
    }
}

void SoundSynthesizer::parseOscillator(const nlohmann::json& osc, SynthesisParams& params) {
    if (osc.contains("types") && osc["types"].is_array()) {
        for (const auto& type : osc["types"]) {
            if (type.is_string()) {
                params.oscillatorTypes.push_back(type.get<std::string>());
            }
        }
    }
    
    if (osc.contains("mix_ratios") && osc["mix_ratios"].is_array()) {
        for (const auto& ratio : osc["mix_ratios"]) {
            if (ratio.is_number()) {
                params.oscillatorMix.push_back(ratio.get<double>());
            }
        }
    }
    
    if (osc.contains("detune") && osc["detune"].is_number()) {
        params.detune = osc["detune"].get<double>();
    }
}

void SoundSynthesizer::parseFilter(const nlohmann::json& filter, SynthesisParams& params) {
    if (filter.contains("type") && filter["type"].is_string()) {
        params.filterType = filter["type"].get<std::string>();
    }
    
    if (filter.contains("cutoff")) {
        if (filter["cutoff"].is_string()) {
            std::string cutoffStr = filter["cutoff"].get<std::string>();
            // Parse frequency strings like "400Hz"
            if (cutoffStr.find("Hz") != std::string::npos) {
                cutoffStr = cutoffStr.substr(0, cutoffStr.find("Hz"));
                params.cutoff = std::stod(cutoffStr);
            }
        } else if (filter["cutoff"].is_number()) {
            params.cutoff = filter["cutoff"].get<double>();
        }
    }
    
    if (filter.contains("resonance") && filter["resonance"].is_number()) {
        params.resonance = filter["resonance"].get<double>();
    }
}

void SoundSynthesizer::parseEffects(const nlohmann::json& effects, SynthesisParams& params) {
    if (effects.is_array()) {
        for (const auto& effect : effects) {
            if (effect.is_object() && effect.contains("type")) {
                std::string type = effect["type"].get<std::string>();
                params.effects.push_back(type);
                
                if (type == "reverb" && effect.contains("amount")) {
                    params.reverbAmount = effect["amount"].get<double>();
                } else if (type == "delay" && effect.contains("amount")) {
                    params.delayAmount = effect["amount"].get<double>();
                } else if (type == "distortion" && effect.contains("amount")) {
                    params.distortionAmount = effect["amount"].get<double>();
                }
            }
        }
    }
}

bool SoundSynthesizer::initializeAudioOutput() {
    // Placeholder for ALSA initialization
    // In a real implementation, this would set up ALSA PCM device
    return true;
}

void SoundSynthesizer::cleanupAudioOutput() {
    // Placeholder for ALSA cleanup
}

int SoundSynthesizer::findFreeVoice() {
    for (int i = 0; i < MAX_POLYPHONY; ++i) {
        if (!voices_[i].isActive()) {
            return i;
        }
    }
    return -1; // No free voice
}

Voice* SoundSynthesizer::findVoiceByNote(int noteNumber) {
    for (auto& voice : voices_) {
        if (voice.isActive() && voice.getNoteNumber() == noteNumber) {
            return &voice;
        }
    }
    return nullptr;
}

double SoundSynthesizer::midiToFrequency(int noteNumber) {
    return 440.0 * std::pow(2.0, (noteNumber - 69) / 12.0);
}

// ============================================================================
// MIDIHandler Implementation
// ============================================================================

MIDIHandler::MIDIHandler(SoundSynthesizer* synthesizer) 
    : synthesizer_(synthesizer), isActive_(false), shouldStop_(false), midiDevice_(nullptr) {
}

MIDIHandler::~MIDIHandler() {
    stopListening();
    cleanupMIDIInput();
}

bool MIDIHandler::initialize() {
    return initializeMIDIInput();
}

bool MIDIHandler::startListening() {
    if (isActive_) return true;
    
    shouldStop_ = false;
    isActive_ = true;
    
    // Start MIDI thread
    midiThread_ = std::thread(&MIDIHandler::midiCallback, this);
    
    return true;
}

void MIDIHandler::stopListening() {
    if (!isActive_) return;
    
    shouldStop_ = true;
    isActive_ = false;
    
    if (midiThread_.joinable()) {
        midiThread_.join();
    }
}

void MIDIHandler::midiCallback() {
    // Placeholder for MIDI input handling
    // In a real implementation, this would read from ALSA MIDI
    while (!shouldStop_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

bool MIDIHandler::initializeMIDIInput() {
    // Placeholder for ALSA MIDI initialization
    return true;
}

void MIDIHandler::cleanupMIDIInput() {
    // Placeholder for ALSA MIDI cleanup
}

// ============================================================================
// SoundSynthesizerManager Implementation
// ============================================================================

SoundSynthesizerManager::SoundSynthesizerManager() 
    : isInitialized_(false) {
}

SoundSynthesizerManager::~SoundSynthesizerManager() {
    stopPlayback();
}

bool SoundSynthesizerManager::initialize() {
    if (isInitialized_) return true;
    
    synthesizer_ = std::make_unique<SoundSynthesizer>();
    if (!synthesizer_->initialize()) {
        std::cerr << "Failed to initialize synthesizer" << std::endl;
        return false;
    }
    
    midiHandler_ = std::make_unique<MIDIHandler>(synthesizer_.get());
    if (!midiHandler_->initialize()) {
        std::cerr << "Failed to initialize MIDI handler" << std::endl;
        return false;
    }
    
    isInitialized_ = true;
    return true;
}

bool SoundSynthesizerManager::playConfiguration(const AudioConfig& config) {
    if (!isInitialized_) {
        std::cerr << "Synthesizer not initialized" << std::endl;
        return false;
    }
    
    if (!synthesizer_->loadConfiguration(config)) {
        std::cerr << "Failed to load configuration" << std::endl;
        return false;
    }
    
    if (!synthesizer_->startPlayback()) {
        std::cerr << "Failed to start playback" << std::endl;
        return false;
    }
    
    if (!midiHandler_->startListening()) {
        std::cerr << "Failed to start MIDI listening" << std::endl;
        return false;
    }
    
    return true;
}

void SoundSynthesizerManager::stopPlayback() {
    if (midiHandler_) {
        midiHandler_->stopListening();
    }
    
    if (synthesizer_) {
        synthesizer_->stopPlayback();
    }
}

} // namespace audio_config