# AI Instrument Synthesis System

A modular, AI-driven system for generating instrument configurations and composite patches with semantic keyword matching, layered composition, and intelligent parameter optimization.

## Overview

This system implements a comprehensive framework for:
- **Semantic Keyword Matching**: AI-driven configuration selection based on user intent
- **Composite Patch Generation**: Multi-layer instrument and effect stacking
- **Interactive Configuration**: User-friendly menu system for parameter selection
- **Type-Safe JSON Processing**: Robust parsing with validation and error handling
- **Modular Architecture**: Extensible design following modern C++ practices

## Architecture

### Core Components

```
include/
├── common_types.h          # Shared type definitions and utilities
├── audio_config.h          # Audio configuration classes
├── ai_scorer.h             # AI semantic scoring and matching
├── patch_generator.h       # Layered patch generation
├── json_parser.h           # Type-safe JSON processing
└── interactive_menu.h      # User interface and menu system

src/
├── main.cpp               # Application entry point
├── audio_config.cpp       # Audio configuration implementation
├── ai_scorer.cpp          # AI scoring implementation
├── patch_generator.cpp    # Patch generation implementation
├── json_parser.cpp        # JSON parsing implementation
└── interactive_menu.cpp   # Menu system implementation
```

### Key Design Principles

1. **Modularity**: Clear separation of concerns with well-defined interfaces
2. **Immutability**: Configuration objects follow immutable design patterns
3. **Type Safety**: Explicit type declarations and compile-time safety
4. **Extensibility**: Plugin-style architecture for future enhancements
5. **Error Handling**: Comprehensive error reporting with context information

## Configuration Files

### Reference Files (For AI Scoring Only)
- `moods.json`: Mood parameters for semantic scoring and vectorization
- `Synthesizer.json`: Synthesis reference data for AI matching

### Output Files (Renderable Configurations)
- `group.json`: Synthesizer and group instrument configurations
- `guitar.json`: Guitar-specific configurations

### AI Configuration
- `skd.json`: Semantic Keyword Database for AI matching (optional)

## Building

### Prerequisites
- C++17 compatible compiler (GCC 8+, Clang 7+, MSVC 2019+)
- CMake 3.16 or later
- nlohmann/json library

### Build Instructions

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build the project
make -j$(nproc)

# Run tests (optional)
make test
```

### Dependencies

The system requires the nlohmann/json library for JSON processing. Install via package manager:

```bash
# Ubuntu/Debian
sudo apt-get install nlohmann-json3-dev

# macOS with Homebrew
brew install nlohmann-json

# Or use vcpkg/conan for cross-platform dependency management
```

## Usage

### Interactive Mode (Default)

```bash
./AIInstrumentSynthesis
```

The interactive menu guides you through:
1. **Section Selection**: Musical section (intro, verse, chorus, etc.)
2. **Mood Selection**: Emotional characteristics (calm, energetic, nostalgic, etc.)
3. **Timbral Selection**: Sound characteristics (warm, bright, dark, etc.)
4. **Instrument Selection**: Instrument preferences (guitar, synth, bass, etc.)
5. **Effect Selection**: Effect preferences (reverb, delay, distortion, etc.)

### Command Line Options

```bash
./AIInstrumentSynthesis [options]

Options:
  --config-dir <dir>      Directory containing JSON config files (default: .)
  --output-dir <dir>      Directory for output files (default: .)
  --skd-file <file>       Semantic keyword database file (default: skd.json)
  --no-interactive        Disable interactive menu mode
  --no-colors             Disable colored console output
  --no-validation         Skip configuration validation on load
  --min-score <score>     Minimum suggestion score (0.0-1.0, default: 0.2)
  --max-suggestions <n>   Maximum suggestions to show (default: 5)
  --verbose, -v           Enable verbose output
  --help, -h              Show help message
```

### Non-Interactive Mode

```bash
./AIInstrumentSynthesis --no-interactive --verbose
```

Generates example patches with predefined parameters for batch processing.

## AI Integration Principles

### Reference vs Output Separation

- **Reference JSONs** (`moods.json`, `Synthesizer.json`): Used exclusively for AI scoring, vectorization, and schema reference
- **Output JSONs** (`group.json`, `guitar.json`): Contain valid, renderable configurations for audio processing

### Layering and Patching

The system implements composite patching with defined layer roles:

```cpp
enum class LayerRole {
    BACKGROUND_TEXTURE,  // Subtle background elements
    AMBIENT_PAD,         // Ambient harmonic content  
    SUPPORTIVE_HARMONY,  // Supporting harmonic elements
    RHYTHMIC_MOTION,     // Rhythmic/percussive elements
    MAIN_MELODIC,        // Primary melodic content
    LEAD_FOREGROUND,     // Lead/solo foreground elements
    EFFECT_LAYER         // Pure effect processing layer
};
```

### AI Scoring Algorithm

The system uses multi-strategy scoring:

1. **Semantic Matching**: Keyword-based similarity scoring
2. **Parameter Similarity**: Numerical parameter comparison
3. **Contextual Bonuses**: Musical context awareness
4. **Quality Weighting**: Configuration quality consideration

### Context-Aware Gain Balancing

Automatic gain balancing considers:
- Layer role hierarchy
- Musical context (section, mood)
- Cross-layer interaction and ducking
- Dynamic range optimization

## Configuration Schema

### Instrument Configuration Structure

```json
{
  "configurationId": "unique_identifier",
  "instrumentType": "guitar_acoustic|synthesizer_subtractive|...",
  "oscillator": {
    "types": ["sine", "sawtooth"],
    "mixRatios": [0.7, 0.3],
    "detune": 0.02
  },
  "envelope": {
    "type": "ADSR",
    "attack": 0.1,
    "decay": 0.2,
    "sustain": 0.8,
    "release": 0.5
  },
  "filter": {
    "type": "low-pass",
    "cutoff": 1000.0,
    "resonance": 0.3
  },
  "effects": [
    {
      "type": "reverb",
      "decay": 2.0,
      "wet": 0.3,
      "aiControl": true
    }
  ],
  "soundCharacteristics": {
    "timbral": "warm",
    "material": "wood",
    "dynamic": "smooth",
    "emotional": [
      {"tag": "calm", "weight": 0.8},
      {"tag": "reflective", "weight": 0.6}
    ]
  },
  "topologicalMetadata": {
    "damping": "medium",
    "spectralComplexity": "simple",
    "manifoldPosition": "intro/outro region"
  }
}
```

### Layer Configuration Output

```json
{
  "patchId": "generated_patch_001",
  "patchName": "Warm Calm Intro",
  "baseInstrument": {
    "configurationId": "acoustic_guitar_warm",
    "gain": 0.8
  },
  "layers": [
    {
      "layerId": "layer_001",
      "role": "AMBIENT_PAD",
      "instrumentConfig": { /* full config */ },
      "gain": 0.4,
      "pan": 0.0,
      "priority": 1
    }
  ],
  "masterGain": 1.0
}
```

## Extending the System

### Adding New Instrument Types

1. **Extend InstrumentType enum** in `common_types.h`
2. **Create specialized configuration class** inheriting from `InstrumentConfig`
3. **Add parsing logic** in `ConfigurationFileLoader`
4. **Update menu choices** in `InteractiveMenuSystem`

### Adding New Effect Types

1. **Register effect schema** using `EffectConfig::registerEffectSchema()`
2. **Add effect-specific parameters** to JSON configurations
3. **Update AI scoring logic** if needed for new semantic properties

### Adding New AI Scoring Strategies

1. **Extend ScoringStrategy enum** in `ai_scorer.h`
2. **Implement scoring logic** in `AiConfigurationScorer::computeSemanticScore()`
3. **Add strategy-specific parameters** to configuration

### Custom Menu Sections

```cpp
menuSystem.addCustomSection(
    "custom_section",
    "Custom Selection",
    "Choose custom parameters",
    {
        MenuChoice("option1", "Option 1", "Description 1"),
        MenuChoice("option2", "Option 2", "Description 2")
    },
    false,  // allowMultiple
    true    // isRequired
);
```

## Testing

### Running Tests

```bash
cd build
make test

# Or run specific test suites
./test/test_audio_config
./test/test_ai_scorer
./test/test_json_parser
```

### Test Structure

```
test/
├── test_common_types.cpp      # Common type utilities tests
├── test_audio_config.cpp      # Audio configuration tests
├── test_ai_scorer.cpp         # AI scoring algorithm tests
├── test_patch_generator.cpp   # Patch generation tests
├── test_json_parser.cpp       # JSON parsing tests
└── test_interactive_menu.cpp  # Menu system tests
```

## Development Guidelines

### Code Style
- **Naming**: PascalCase for classes, camelCase for functions/variables, ALL_CAPS for constants
- **Documentation**: Doxygen-style comments for all public APIs
- **Error Handling**: Use exceptions for unexpected errors, return optional/result types for expected failures
- **Memory Management**: Prefer smart pointers and RAII principles

### Adding Features
1. Follow the established module structure
2. Add comprehensive tests for new functionality
3. Update documentation and examples
4. Ensure backward compatibility with existing configurations

### Performance Considerations
- JSON parsing is optimized for configuration loading (not real-time)
- AI scoring algorithms are designed for interactive latency (< 100ms)
- Memory usage is optimized through shared_ptr for configurations
- Consider caching for frequently accessed AI computations

## Troubleshooting

### Common Issues

**Configuration Loading Errors**
- Check JSON syntax and schema validation
- Ensure all required fields are present
- Verify file paths and permissions

**AI Scoring Issues**  
- Validate semantic keyword database
- Check scoring strategy configuration
- Ensure sufficient valid configurations are loaded

**Build Issues**
- Verify C++17 compiler support
- Check nlohmann/json library installation
- Ensure CMake version compatibility

### Debug Mode

Run with verbose output for detailed information:

```bash
./AIInstrumentSynthesis --verbose --config-dir ./configs
```

## License

This project is released under the MIT License. See LICENSE file for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Ensure all tests pass
5. Submit a pull request with clear description

## Future Enhancements

- **Vector Embeddings**: Advanced semantic similarity using word2vec/transformer models
- **Real-time Parameter Modulation**: Dynamic AI-controlled parameter changes
- **Machine Learning Integration**: Training models on user preferences
- **Plugin Architecture**: Support for external instrument/effect plugins
- **Web Interface**: Browser-based configuration and preview system
- **MIDI Integration**: Real-time MIDI input and output support