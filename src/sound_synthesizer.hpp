/**
 * @file sound_synthesizer.hpp
 * @brief Real-time Sound Synthesizer Engine
 * @author AI Assistant
 * @version 1.0
 * 
 * Integrates with the Multi-Dimensional Audio Configuration System to provide
 * real-time audio synthesis based on user-selected configurations.
 * 
 * FEATURES:
 * - Real-time audio synthesis from JSON configurations
 * - MIDI input support for note triggering
 * - Multiple synthesis methods (subtractive, FM, wavetable)
 * - ADSR envelope generation
 * - Filter and effects processing
 * - Polyphonic playback
 * - Audio output via ALSA/PulseAudio
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <mutex>
#include <queue>
#include <functional>
#include <cmath>
#include <array>
#include "audio_config_system.hpp"
#include "json.hpp"

namespace audio_config {

// Audio constants
constexpr double SAMPLE_RATE = 44100.0;
constexpr int BUFFER_SIZE = 512;
constexpr int MAX_POLYPHONY = 16;

/**
 * @brief MIDI note information
 */
struct MIDINote {
    int noteNumber;        // 0-127 (MIDI standard)
    double frequency;      // Calculated frequency in Hz
    double velocity;       // 0.0-1.0
    double startTime;      // Time when note was pressed
    bool isActive;         // Whether note is currently playing
    
    MIDINote(int note, double vel, double time) 
        : noteNumber(note), velocity(vel), startTime(time), isActive(true) {
        // Calculate frequency from MIDI note number
        frequency = 440.0 * std::pow(2.0, (noteNumber - 69) / 12.0);
    }
};

/**
 * @brief Audio synthesis parameters
 */
struct SynthesisParams {
    // Oscillator parameters
    std::vector<std::string> oscillatorTypes;
    std::vector<double> oscillatorMix;
    double detune = 0.0;
    
    // ADSR envelope
    double attack = 0.1;   // seconds
    double decay = 0.1;    // seconds
    double sustain = 0.7;  // 0.0-1.0
    double release = 0.5;  // seconds
    
    // Filter parameters
    std::string filterType = "lowpass";
    double cutoff = 1000.0;  // Hz
    double resonance = 0.0;  // 0.0-1.0
    
    // Effects
    std::vector<std::string> effects;
    double reverbAmount = 0.0;
    double delayAmount = 0.0;
    double distortionAmount = 0.0;
    
    // Global parameters
    double volume = 0.8;   // 0.0-1.0
    double pan = 0.0;      // -1.0 (left) to 1.0 (right)
};

/**
 * @brief Voice state for polyphonic synthesis
 */
class Voice {
public:
    Voice() : isActive_(false), noteNumber_(0), phase_(0.0), envelopePhase_(0.0) {}
    
    void startNote(const MIDINote& note, const SynthesisParams& params);
    void stopNote();
    void update(double deltaTime, const SynthesisParams& params);
    double generateSample(const SynthesisParams& params);
    
    bool isActive() const { return isActive_; }
    int getNoteNumber() const { return noteNumber_; }
    
private:
    bool isActive_;
    int noteNumber_;
    double frequency_;
    double velocity_;
    double phase_;
    double envelopePhase_;
    double envelopeValue_;
    double envelopeTarget_;
    double envelopeRate_;
    
    // Envelope state machine
    enum class EnvelopeState { Attack, Decay, Sustain, Release, Off };
    EnvelopeState envelopeState_;
    
    // Helper methods
    double generateOscillator(const SynthesisParams& params);
    double applyFilter(double sample, const SynthesisParams& params);
    double applyEffects(double sample, const SynthesisParams& params);
    void updateEnvelope(double deltaTime, const SynthesisParams& params);
};

/**
 * @brief Main sound synthesizer engine
 */
class SoundSynthesizer {
public:
    SoundSynthesizer();
    ~SoundSynthesizer();
    
    /**
     * @brief Initialize the synthesizer
     * @return true if successful
     */
    bool initialize();
    
    /**
     * @brief Load configuration and start synthesis
     * @param config Audio configuration to synthesize
     * @return true if successful
     */
    bool loadConfiguration(const AudioConfig& config);
    
    /**
     * @brief Start audio playback
     * @return true if successful
     */
    bool startPlayback();
    
    /**
     * @brief Stop audio playback
     */
    void stopPlayback();
    
    /**
     * @brief Check if synthesizer is running
     * @return true if active
     */
    bool isRunning() const { return isRunning_; }
    
    /**
     * @brief Trigger a MIDI note
     * @param noteNumber MIDI note number (0-127)
     * @param velocity Note velocity (0.0-1.0)
     */
    void noteOn(int noteNumber, double velocity);
    
    /**
     * @brief Release a MIDI note
     * @param noteNumber MIDI note number (0-127)
     */
    void noteOff(int noteNumber);
    
    /**
     * @brief Stop all notes
     */
    void allNotesOff();
    
    /**
     * @brief Set global volume
     * @param volume Volume level (0.0-1.0)
     */
    void setVolume(double volume);
    
    /**
     * @brief Get current synthesis parameters
     * @return Reference to current parameters
     */
    const SynthesisParams& getParameters() const { return currentParams_; }
    
    /**
     * @brief Update synthesis parameters in real-time
     * @param params New parameters
     */
    void updateParameters(const SynthesisParams& params);

private:
    // Audio processing
    std::array<Voice, MAX_POLYPHONY> voices_;
    SynthesisParams currentParams_;
    std::atomic<bool> isRunning_;
    std::atomic<double> globalVolume_;
    
    // Audio thread
    std::thread audioThread_;
    std::atomic<bool> shouldStop_;
    std::mutex audioMutex_;
    
    // MIDI input
    std::queue<MIDINote> noteQueue_;
    std::mutex noteMutex_;
    
    // Audio output (platform-specific)
    void* audioDevice_;  // ALSA/PulseAudio device handle
    
    // Core audio processing
    void audioCallback(float* outputBuffer, int numFrames);
    void processAudio();
    
    // Configuration parsing
    SynthesisParams parseConfiguration(const AudioConfig& config);
    void parseADSR(const nlohmann::json& adsr, SynthesisParams& params);
    void parseOscillator(const nlohmann::json& osc, SynthesisParams& params);
    void parseFilter(const nlohmann::json& filter, SynthesisParams& params);
    void parseEffects(const nlohmann::json& effects, SynthesisParams& params);
    
    // Platform-specific audio initialization
    bool initializeAudioOutput();
    void cleanupAudioOutput();
    
    // Helper methods
    int findFreeVoice();
    Voice* findVoiceByNote(int noteNumber);
    double midiToFrequency(int noteNumber);
};

/**
 * @brief MIDI input handler
 */
class MIDIHandler {
public:
    MIDIHandler(SoundSynthesizer* synthesizer);
    ~MIDIHandler();
    
    /**
     * @brief Initialize MIDI input
     * @return true if successful
     */
    bool initialize();
    
    /**
     * @brief Start listening for MIDI events
     * @return true if successful
     */
    bool startListening();
    
    /**
     * @brief Stop listening for MIDI events
     */
    void stopListening();
    
    /**
     * @brief Check if MIDI is active
     * @return true if listening
     */
    bool isActive() const { return isActive_; }

private:
    SoundSynthesizer* synthesizer_;
    std::atomic<bool> isActive_;
    std::thread midiThread_;
    std::atomic<bool> shouldStop_;
    
    // Platform-specific MIDI handling
    void* midiDevice_;  // ALSA MIDI device handle
    
    void midiCallback();
    bool initializeMIDIInput();
    void cleanupMIDIInput();
};

/**
 * @brief Sound synthesizer manager
 * 
 * Provides high-level interface for integrating synthesizer with AudioConfigSystem
 */
class SoundSynthesizerManager {
public:
    SoundSynthesizerManager();
    ~SoundSynthesizerManager();
    
    /**
     * @brief Initialize the synthesizer system
     * @return true if successful
     */
    bool initialize();
    
    /**
     * @brief Play a selected audio configuration
     * @param config Audio configuration to play
     * @return true if successful
     */
    bool playConfiguration(const AudioConfig& config);
    
    /**
     * @brief Stop current playback
     */
    void stopPlayback();
    
    /**
     * @brief Check if synthesizer is ready
     * @return true if initialized
     */
    bool isReady() const { return synthesizer_ && synthesizer_->isRunning(); }
    
    /**
     * @brief Get synthesizer instance
     * @return Pointer to synthesizer
     */
    SoundSynthesizer* getSynthesizer() { return synthesizer_.get(); }

private:
    std::unique_ptr<SoundSynthesizer> synthesizer_;
    std::unique_ptr<MIDIHandler> midiHandler_;
    bool isInitialized_;
};

} // namespace audio_config