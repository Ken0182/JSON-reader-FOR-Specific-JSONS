# Sound Synthesizer Linkage - Architecture & Implementation Plan

**Project**: Multi-Dimensional Audio Configuration System  
**Version**: 1.7 (Sound Synthesis Extension)  
**Date**: 2025-10-27  
**Status**: Planning Phase

---

## Executive Summary

This document outlines the architecture and implementation plan for linking the existing multi-dimensional audio configuration system with a sound synthesizer that plays sounds based on user selections. The system will enable users to:

1. **Search** for sounds using semantic queries (e.g., "warm bass", "dreamy pad")
2. **Select** configurations through the CLI or API
3. **Play** the corresponding synthesized sound in real-time
4. **Experiment** with different parameters and hear immediate audio feedback

---

## Current System Overview

### Existing Components

The system already provides:

1. **SemanticKnowledgeBase** (v1.6)
   - SQLite database with 366 tags and embeddings
   - Semantic search with unlimited vocabulary
   - Contrastive queries (positive/negative constraints)

2. **AudioConfigSystem**
   - Multi-dimensional compatibility analysis
   - Interactive CLI with search, select, boost, demote commands
   - Configuration generation for synthesis

3. **Configuration Database**
   - Clean, validated JSON configs with sound characteristics
   - ADSR envelopes, filters, effects, emotional tags
   - Synthesizer parameters (see `Synthesizer.json`)

### Key Data Structures

From `Synthesizer.json`, we have:
```json
{
  "oscillator": ["sine", "triangle", "sawtooth", "square", "fm", "wavetable"],
  "adsr": {
    "attack": [0.0-1.0],
    "decay": [0.0-1.0], 
    "sustain": [0.0-1.0],
    "release": [0.0-3.0]
  },
  "filter": "low-pass | high-pass | band-pass | notch",
  "fx": ["reverb", "delay", "chorus", "distortion", "flanger"],
  "emotion": "calm | energetic | tense | reflective"
}
```

---

## Architecture Design

### Component Hierarchy

```
┌─────────────────────────────────────────────────────────────┐
│                    User Interface Layer                      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │  CLI Input   │  │ GUI (future) │  │  HTTP API    │      │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘      │
└─────────┼──────────────────┼──────────────────┼─────────────┘
          │                  │                  │
          └──────────────────┴──────────────────┘
                             │
┌────────────────────────────┴─────────────────────────────────┐
│              Audio Configuration System (existing)            │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  • Semantic search (SemanticKnowledgeBase)           │   │
│  │  • Multi-dimensional pointer                         │   │
│  │  • Configuration selection & boost/demote            │   │
│  │  • User context tracking                             │   │
│  └──────────────────────────────────────────────────────┘   │
└────────────────────────────┬─────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────┐
│         **NEW**: Sound Synthesis Engine Layer               │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  SoundSynthesizer (C++)                              │  │
│  │  ┌────────────────────────────────────────────────┐  │  │
│  │  │  • Config → Sound Parameter Mapper             │  │  │
│  │  │  • Oscillator bank (sine, saw, square, etc.)   │  │  │
│  │  │  • ADSR envelope generator                     │  │  │
│  │  │  • Filter bank (LP, HP, BP, notch)            │  │  │
│  │  │  • Effects chain (reverb, delay, chorus)      │  │  │
│  │  │  • Real-time parameter modulation             │  │  │
│  │  └────────────────────────────────────────────────┘  │  │
│  └──────────────────────────────────────────────────────┘  │
└────────────────────────────┬─────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────┐
│              Audio Output Layer                              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │  PortAudio   │  │    ALSA      │  │  CoreAudio   │      │
│  │  (cross-plat)│  │   (Linux)    │  │   (macOS)    │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
```

---

## Design Decisions

### 1. Audio Library Choice: **PortAudio**

**Rationale:**
- Cross-platform (Linux, macOS, Windows)
- Low-latency real-time audio
- Simple C API, well-documented
- Mature, stable, widely used

**Alternatives Considered:**
- RtAudio (similar, but less common)
- JACK (Linux-focused, more complex)
- Direct ALSA/CoreAudio (platform-specific)

### 2. Synthesis Architecture: **Modular DSP Pipeline**

```
Config JSON → Parameter Mapper → [Oscillator → ADSR → Filter → FX] → Output
```

**Key Components:**

#### a) Parameter Mapper
- Converts semantic tags to DSP parameters
- Example: "warm" → lower cutoff, softer attack
- Example: "bright" → higher cutoff, faster attack
- Uses embedding similarity for fuzzy matching

#### b) Oscillator Bank
- Basic waveforms: sine, triangle, sawtooth, square
- Advanced: FM synthesis, wavetable
- Polyphonic (up to 16 voices)
- MIDI note input support

#### c) ADSR Envelope
- Attack, Decay, Sustain, Release
- Per-voice envelope tracking
- Configurable curve shapes

#### d) Filter Bank
- Biquad filters: LP, HP, BP, notch
- Resonance control
- Envelope modulation support

#### e) Effects Chain
- Reverb (Schroeder/Freeverb algorithm)
- Delay (with feedback)
- Chorus (modulated delay)
- Distortion (soft clipping)

### 3. Integration Strategy: **Hybrid Approach**

**Option A**: Embedded in AudioConfigSystem (chosen)
- Add `SoundSynthesizer` class to existing system
- CLI commands: `play <config_id>`, `play_search <query>`
- Real-time parameter adjustment

**Option B**: Standalone synthesizer executable
- Separate binary reads config JSON
- Simpler, but requires IPC

---

## Implementation Plan

### Phase 1: Audio Foundation (Week 1)

**Goal**: Basic audio output and oscillator

**Tasks**:
1. ✅ Set up PortAudio dependency
   - Add to Makefile: `-lportaudio`
   - Test audio output callback

2. ✅ Implement basic oscillator
   - File: `src/audio_synthesizer.hpp`, `src/audio_synthesizer.cpp`
   - Classes: `Oscillator`, `WaveformGenerator`
   - Waveforms: sine, sawtooth, square, triangle

3. ✅ Create audio callback infrastructure
   - Real-time safe buffer filling
   - Voice management (mono → stereo)

**Deliverable**: CLI command `test-tone` plays a 440Hz sine wave

---

### Phase 2: ADSR Envelope (Week 1)

**Goal**: Dynamic envelope control

**Tasks**:
1. ✅ Implement ADSR envelope generator
   - File: `src/adsr_envelope.hpp`, `src/adsr_envelope.cpp`
   - States: Attack, Decay, Sustain, Release
   - Note on/off triggering

2. ✅ Integrate envelope with oscillator
   - Per-voice envelope tracking
   - Amplitude modulation

**Deliverable**: CLI command `play-note 60 1000` plays middle C for 1 second with envelope

---

### Phase 3: Config Integration (Week 2)

**Goal**: Map AudioConfig to synthesis parameters

**Tasks**:
1. ✅ Create parameter mapper
   - File: `src/synthesis_mapper.hpp`, `src/synthesis_mapper.cpp`
   - Function: `AudioConfig → SynthesisParameters`
   - Semantic tag → DSP parameter mapping

2. ✅ Extract parameters from config JSON
   - Parse ADSR values
   - Parse oscillator type
   - Parse filter settings

3. ✅ Implement CLI command: `play <config_id>`
   - Look up config by ID
   - Map to synthesis params
   - Trigger sound playback

**Deliverable**: `play Lead_Bright_Energetic` plays corresponding sound

---

### Phase 4: Filters & Effects (Week 2)

**Goal**: Add timbre control and spatial effects

**Tasks**:
1. ✅ Implement biquad filter
   - File: `src/audio_filter.hpp`, `src/audio_filter.cpp`
   - Types: low-pass, high-pass, band-pass, notch
   - Cutoff and resonance parameters

2. ✅ Implement reverb effect
   - Freeverb or Schroeder reverb algorithm
   - Room size, damping, mix parameters

3. ✅ Implement delay effect
   - Circular buffer
   - Feedback and mix controls

**Deliverable**: `play Pad_Warm_Calm` has warm filter and reverb

---

### Phase 5: Semantic Search Integration (Week 3)

**Goal**: Play sounds from semantic queries

**Tasks**:
1. ✅ Implement `play-search <query>` command
   - Search configurations by query
   - Play top result automatically
   - Option: `play-search "dreamy not harsh" --preview`

2. ✅ Add interactive preview mode
   - Search → Display results → User selects → Play
   - Arrow keys for navigation
   - Space bar to play/pause

3. ✅ Add real-time parameter tweaking
   - While playing, adjust ADSR, filter, effects
   - Commands: `adjust attack 0.5`, `adjust cutoff 800`

**Deliverable**: Full semantic search → sound playback workflow

---

### Phase 6: Advanced Features (Week 3-4)

**Goal**: Polyphony, MIDI, recording

**Tasks**:
1. ⬜ Polyphonic playback (up to 16 voices)
   - Voice allocation strategy
   - Voice stealing (oldest note)

2. ⬜ MIDI input support
   - Listen for MIDI events
   - Map MIDI note to config playback

3. ⬜ Recording/export
   - Render to WAV file
   - Command: `record <config_id> output.wav`

4. ⬜ Performance optimization
   - SIMD for oscillator/filter
   - Lock-free audio thread communication

**Deliverable**: Production-ready synthesis engine

---

## API Design

### New Classes

#### 1. `SoundSynthesizer`

```cpp
namespace audio_config {

class SoundSynthesizer {
public:
    SoundSynthesizer();
    ~SoundSynthesizer();
    
    // Initialize audio system
    bool initialize(int sampleRate = 44100, int bufferSize = 256);
    
    // Playback control
    void playConfig(const AudioConfig& config, float durationSeconds = 2.0f);
    void playNote(int midiNote, float velocity, float durationSeconds = 1.0f);
    void stop();
    bool isPlaying() const;
    
    // Real-time parameter adjustment
    void setOscillatorType(OscillatorType type);
    void setADSR(float attack, float decay, float sustain, float release);
    void setFilterCutoff(float cutoff);
    void setFilterResonance(float resonance);
    void setReverbMix(float mix);
    void setDelayTime(float timeMs);
    
    // State query
    SynthesisParameters getCurrentParameters() const;
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace audio_config
```

#### 2. `SynthesisMapper`

```cpp
namespace audio_config {

struct SynthesisParameters {
    OscillatorType oscillator = OscillatorType::Sawtooth;
    float attackMs = 10.0f;
    float decayMs = 200.0f;
    float sustainLevel = 0.7f;
    float releaseMs = 500.0f;
    FilterType filter = FilterType::LowPass;
    float filterCutoff = 2000.0f;
    float filterResonance = 0.5f;
    float reverbMix = 0.3f;
    float delayTimeMs = 250.0f;
    float delayFeedback = 0.4f;
};

class SynthesisMapper {
public:
    static SynthesisParameters mapConfig(const AudioConfig& config);
    
private:
    static OscillatorType mapOscillator(const std::string& oscType);
    static FilterType mapFilter(const std::string& filterType);
    static void applySemanticAdjustments(SynthesisParameters& params, 
                                         const std::vector<std::string>& tags);
};

} // namespace audio_config
```

---

## CLI Command Extensions

### New Commands

```bash
# Play a configuration by ID
> play Lead_Bright_Energetic

# Play top result from semantic search
> play-search "warm bass not aggressive"

# Play with custom duration
> play Pad_Warm_Calm --duration 5.0

# Test tone
> test-tone 440 sine 2.0

# Interactive preview mode
> preview "dreamy ambient"
  [Shows search results, press 1-9 to play]

# Adjust parameters in real-time (while playing)
> adjust attack 0.5
> adjust cutoff 1200
> adjust reverb 0.6

# Record to file
> record Lead_Bright_Energetic output.wav --duration 3.0

# Stop playback
> stop
```

---

## File Structure

### New Files

```
src/
├── audio_synthesizer.hpp          (Main synthesizer class)
├── audio_synthesizer.cpp
├── synthesis_mapper.hpp           (Config → Params mapper)
├── synthesis_mapper.cpp
├── oscillator.hpp                 (Waveform generators)
├── oscillator.cpp
├── adsr_envelope.hpp              (Envelope generator)
├── adsr_envelope.cpp
├── audio_filter.hpp               (Biquad filters)
├── audio_filter.cpp
├── audio_effects.hpp              (Reverb, delay, chorus)
├── audio_effects.cpp
└── audio_utils.hpp                (DSP utility functions)
```

### Modified Files

```
src/
├── audio_config_system.hpp        (Add synthesizer member)
├── audio_config_system.cpp        (Add play commands)
└── audio_config_cli.cpp           (Add CLI handlers)

Makefile                           (Add -lportaudio, link new objects)
```

---

## Dependencies

### Required Libraries

1. **PortAudio** (audio I/O)
   ```bash
   # Ubuntu/Debian
   sudo apt-get install portaudio19-dev
   
   # macOS
   brew install portaudio
   ```

2. **Existing** (already in project)
   - SQLite3
   - C++17 standard library

### Optional Libraries (future)

- **RtMidi** (MIDI input)
- **libsndfile** (WAV export)

---

## Testing Strategy

### Unit Tests

1. Oscillator output correctness
   - Verify waveform values
   - Check frequency accuracy

2. ADSR envelope shape
   - Verify stage transitions
   - Check timing accuracy

3. Filter frequency response
   - Verify cutoff behavior
   - Check resonance peaks

### Integration Tests

1. Config → Sound pipeline
   - Load config → Generate params → Synthesize audio
   - Verify no crashes, audio is generated

2. CLI command execution
   - Test all new commands
   - Verify error handling

### Manual Testing

1. Subjective audio quality
   - "Does 'warm' actually sound warm?"
   - "Is 'bright' actually bright?"

2. Latency measurement
   - CLI command → audio output delay
   - Target: < 50ms

---

## Success Criteria

### Phase 1 Complete When:
- [ ] PortAudio successfully outputs test tone
- [ ] Basic oscillators generate correct waveforms
- [ ] No audio glitches or dropouts

### Phase 2 Complete When:
- [ ] ADSR envelope shapes sound natural
- [ ] Note on/off transitions are smooth
- [ ] Multi-voice playback works (polyphony)

### Phase 3 Complete When:
- [ ] `play <config_id>` command works for all configs
- [ ] Config parameters correctly map to DSP
- [ ] Sound matches semantic tags (subjective)

### Phase 4 Complete When:
- [ ] Filters change timbre noticeably
- [ ] Reverb adds spatial depth
- [ ] Effects are artifact-free

### Phase 5 Complete When:
- [ ] `play-search` finds and plays correct sounds
- [ ] Interactive preview mode is intuitive
- [ ] Real-time adjustments work smoothly

### Phase 6 Complete When:
- [ ] Polyphony supports 16 simultaneous notes
- [ ] MIDI input triggers sounds correctly
- [ ] WAV export produces clean audio files

---

## Risk Mitigation

### Risk 1: Audio Latency
**Impact**: High  
**Probability**: Medium  
**Mitigation**: 
- Use low buffer sizes (256 samples)
- Lock-free audio thread design
- Benchmark on target hardware

### Risk 2: Parameter Mapping Subjectivity
**Impact**: Medium  
**Probability**: High  
**Mitigation**:
- User testing with musicians
- Tunable mapping coefficients
- Machine learning for tag → param (future)

### Risk 3: Cross-Platform Audio Issues
**Impact**: Medium  
**Probability**: Medium  
**Mitigation**:
- Test on Linux, macOS, Windows
- Fallback to simpler audio backends
- Containerized test environments

---

## Future Extensions

### v2.0: Machine Learning
- Train neural network: semantic tags → DSP parameters
- User feedback loop for parameter optimization
- Style transfer (e.g., "make this sound like X")

### v2.1: GUI
- Web-based interface (WebAssembly?)
- Visual parameter editors
- Waveform/spectrum visualization

### v2.2: Plugin Generation
- Export as VST3/AU plugin
- Standalone DAW integration
- Cloud-based sound library

---

## Appendix A: Semantic Tag → DSP Mapping Examples

| Semantic Tag | Oscillator | ADSR (A/D/S/R) | Filter | Effects |
|--------------|------------|----------------|---------|---------|
| **warm**     | triangle   | 50/300/0.7/800 | LP@800Hz | reverb@0.4 |
| **bright**   | sawtooth   | 10/150/0.8/300 | HP@1200Hz | - |
| **dreamy**   | sine       | 500/500/0.6/2000 | LP@600Hz | reverb@0.7 |
| **aggressive** | square   | 5/100/0.9/200  | BP@2000Hz | distortion |
| **punchy**   | sawtooth   | 1/50/0.8/100   | LP@400Hz | - |
| **soft**     | sine       | 300/500/0.5/1000 | LP@1000Hz | reverb@0.5 |

---

## Appendix B: Audio Callback Pseudocode

```cpp
int audioCallback(const void* inputBuffer,
                  void* outputBuffer,
                  unsigned long framesPerBuffer,
                  const PaStreamCallbackTimeInfo* timeInfo,
                  PaStreamCallbackFlags statusFlags,
                  void* userData) {
    
    auto* synth = static_cast<SoundSynthesizer::Impl*>(userData);
    auto* out = static_cast<float*>(outputBuffer);
    
    for (unsigned long i = 0; i < framesPerBuffer; ++i) {
        float sample = 0.0f;
        
        // Generate each active voice
        for (auto& voice : synth->voices) {
            if (!voice.active) continue;
            
            // Oscillator
            float osc = voice.oscillator.generateSample();
            
            // ADSR envelope
            float env = voice.envelope.getNextSample();
            
            // Apply envelope
            sample += osc * env * voice.velocity;
        }
        
        // Apply filter
        sample = synth->filter.process(sample);
        
        // Apply effects
        sample = synth->reverb.process(sample);
        sample = synth->delay.process(sample);
        
        // Output (stereo)
        *out++ = sample;  // left
        *out++ = sample;  // right
    }
    
    return paContinue;
}
```

---

## Appendix C: Build Instructions

### Updated Makefile

```makefile
# Add PortAudio
LDFLAGS = -pthread -lsqlite3 -lportaudio

# New source files
SYNTH_SOURCES = $(SRC_DIR)/audio_synthesizer.cpp \
                $(SRC_DIR)/synthesis_mapper.cpp \
                $(SRC_DIR)/oscillator.cpp \
                $(SRC_DIR)/adsr_envelope.cpp \
                $(SRC_DIR)/audio_filter.cpp \
                $(SRC_DIR)/audio_effects.cpp

SOURCES += $(SYNTH_SOURCES)

# Build target remains the same
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@
```

### Build & Run

```bash
# Install dependencies
sudo apt-get install portaudio19-dev libsqlite3-dev

# Build
make clean && make

# Run with audio
./build/audio_config_system config/weights.json
> play-search "warm bass"
```

---

## Conclusion

This plan provides a comprehensive roadmap for integrating sound synthesis into the multi-dimensional audio configuration system. The modular architecture allows for:

1. **Incremental development** (6 phases)
2. **Testable components** (unit, integration, manual)
3. **Cross-platform support** (PortAudio)
4. **Real-time performance** (lock-free audio thread)
5. **Semantic search integration** (leverages existing infrastructure)

The end result will be a powerful tool for:
- **Sound designers**: Find and preview sounds using natural language
- **Musicians**: Quickly audition presets in context
- **Developers**: Understand semantic → acoustic relationships

**Next Steps**: Begin Phase 1 implementation (audio foundation).

---

**Document Version**: 1.0  
**Last Updated**: 2025-10-27  
**Reviewed By**: System Architect  
**Approved By**: Project Lead
