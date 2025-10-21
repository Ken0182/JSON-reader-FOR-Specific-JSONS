# Multi-Dimensional Audio Configuration System

**Version 1.5** - A comprehensive C++17 system for intelligent audio configuration assembly using **4-dimensional pointing** with **semantic knowledge embeddings**, **unified tokenization**, and **persistent interest tracking** to recommend compatible, musically meaningful, and technically sound instrument/effect combinations.

## What's New

### v1.5 - Search Interest Tracking with Decay 🎯
- 📈 **Persistent tracking** - Learns from your search patterns over time
- ⏰ **Temporal decay** - Interests naturally fade (2-hour half-life)
- 🔄 **EMA smoothing** - Stable, gradual adaptation to preferences
- 💡 **Gentle bias** - Subtle improvements (max 15% boost, tunable)
- ⚙️ **Tunable parameters** - Customize decay, smoothing, bias strength
- 💾 **Export/import** - Persist your interest profile across sessions
- 🛠️ **Comprehensive CLI** - Full `signals` command family (on/off, history, tune, export, import)

### v1.4 - Unified Tokenization & Scoring 🔍
- 🎯 **Per-token matching** - "funky retro" now finds "RetroFunky" (fixed!)
- 🔤 **camelCase/snake_case splitting** - Properly tokenizes identifiers
- 🌍 **Unicode normalization** - Strips diacritics, handles accents
- 📊 **Aligned embeddings** - Search and semantics use same token stream
- ✅ **Semantic validation** - Re-ranks with cosine-on-shared-tokens threshold

### v1.3 - SKD Embedding Integration 🎯
- 🧠 **Semantic Knowledge Database (SKD)** - Real embeddings replace hash-based vectors
- 🎨 **Meaningful similarity** - "warm" ≈ "soft", "bright" ≈ "crisp" (true semantics!)
- 📚 **Unlimited vocabulary** - Load any embedding index (Word2Vec, GloVe, FastText)
- ✅ **Auto-detection** - Automatically loads `data/skd_embeddings.json` if present
- 🔄 **Graceful fallback** - Works without SKD (uses built-in vocabulary)

### v1.2 - Efficiency & Quality Upgrades
- ⚡ **3-4x faster** semantic similarity (pre-normalized embeddings)
- 🚀 **10x faster** tag matching (cached hash sets vs nested loops)
- 📊 **IDF-weighted** tag boost for higher quality matches
- ✅ **Score clamping** to [0,1] range (fixes >1.0 issue)
- 🎯 **Diagonal weighting** support for semantic dimensions

### v1.1 - Startup Bug Fix
- 🔧 Auto-detects resource paths from any directory
- 🏗️ Works from repository root OR build/ directory
- ⚙️ CLI overrides: `--weights <path>` `--config <path>`

**Documentation:**
- `SIGNALS_TRACKING.md` - v1.5 interest tracking with decay
- `TOKENIZATION_UPGRADE.md` - v1.4 unified tokenization system
- `SKD_EMBEDDING_UPGRADE.md` - v1.3 SKD integration details
- `EFFICIENCY_UPGRADES.md` - v1.2 performance optimizations
- `STARTUP_BUGFIX.md` - v1.1 path auto-detection

## Key Features

### 4-Dimensional Pointing System
- **1D Semantic**: FastText-style embeddings for tag/keyword similarity
- **2D Technical**: Real-world compatibility (sample rates, plugin formats, envelope types)
- **3D Musical Role**: Lead/bass/pad classification with typical combinations
- **4D Layering**: Frequency ranges, stereo placement, arrangement context

### AI-Driven Configuration Assembly
- Intelligent search with semantic understanding
- Multi-dimensional compatibility analysis
- Automatic conflict detection and resolution suggestions
- Real-time configuration validation

### Professional Audio Standards
- VST/VST3/AU/AAX plugin format support
- DAW compatibility (Ableton, Logic, Cubase, Pro Tools)
- Sample rate and buffer size validation
- MIDI specification compliance

### Interactive Learning System
- User preference learning (boost/demote)
- Adaptive suggestions based on history
- Explainable AI with detailed reasoning
- Rich feedback and guidance

## Architecture

```
Single Consolidated Binary: audio_config_system
├── Modular Source Code:
│   ├── src/main.cpp                  # Entry point
│   ├── src/audio_config_system.hpp   # Core definitions  
│   ├── src/audio_config_system.cpp   # Core implementation
│   └── src/audio_config_cli.cpp      # CLI commands
├── Configuration:
│   └── config/weights.json           # Tunable parameters
├── Data:
│   └── data/clean_config.json        # Source configurations
└── Build System:
    └── Makefile                      # Cross-platform build
```

## Quick Start

### Prerequisites
- **g++** with C++17 support (including `<filesystem>`)
- **make** utility
- **curl** (for downloading dependencies)

### Build & Setup

```bash
# Clone and navigate to repository
git clone <repository>
cd multi-dimensional-audio-system

# Setup dependencies and build
make setup
make

# Run the application (works from any directory!)
make run

# Or run directly from repository root
./build/audio_config_system

# Or run from build/ directory
cd build && ./audio_config_system
```

### NEW in v1.1: Auto-Detection & CLI Overrides

The system now automatically detects resource paths! Works from:
- Repository root directory
- Build directory (`cd build && ./audio_config_system`)
- Any location with custom paths

```bash
# Show help and options
./build/audio_config_system --help

# Use custom resource paths
./build/audio_config_system \
  --weights /path/to/weights.json \
  --config /path/to/clean_config.json
```

See `STARTUP_BUGFIX.md` for complete details on the path auto-detection system.

### Windows Users (MINGW/MSYS2)

See `WINDOWS_BUILD.md` for detailed Windows build instructions.

```bash
# Build on Windows
make clean
make

# Run from anywhere - auto-detection works!
./build/audio_config_system.exe
cd build && ./audio_config_system.exe
```

## Usage

### Interactive CLI

The system can be launched from multiple locations (auto-detection handles paths):

```bash
# From repository root
./build/audio_config_system

# From build/ directory (NEW in v1.1!)
cd build && ./audio_config_system

# With custom paths
./build/audio_config_system --weights custom/weights.json --config custom/config.json

# Show help
./build/audio_config_system --help
```

All methods produce the same output:

```
Multi-Dimensional Audio Configuration System
=================================================================
Loaded 30 configurations with multi-dimensional metadata.

=== INTERACTIVE SESSION ===
Commands: search, select, boost, demote, exclude, list, stats, generate, help, examples, quit

> 
```

### Example Session

```bash
> search warm guitar
Searching for: "warm guitar"
Found 3 matching configurations:

1. Acoustic_Warm_Fingerstyle (Score: 0.92) [Lead, warm] Tags: warm, organic, intimate
2. Classical_Nylon_Soft (Score: 0.78) [Lead, warm] Tags: soft, classical, smooth
3. Jazz_Hollow_Body (Score: 0.71) [Lead, warm] Tags: jazz, warm, mellow

Use 'select <config_id>' to add to your selection
Use 'boost <config_id>' if you like a result

> select Acoustic_Warm_Fingerstyle
Selected: Acoustic_Warm_Fingerstyle

> search bass punchy
Searching for: "bass punchy"
Found 2 matching configurations:

1. Bass_Classic_MoogPunch (Score: 0.87) [Bass, neutral] Tags: punchy, classic, powerful
2. Bass_Deep_Sub (Score: 0.74) [Bass, dark] Tags: deep, sub, powerful

> select Bass_Classic_MoogPunch
Selected: Bass_Classic_MoogPunch

Compatibility with existing selections:
  Acoustic_Warm_Fingerstyle: 0.85 (recommended)

> list
Selected Configurations (2):
1. Acoustic_Warm_Fingerstyle [Lead, warm] Tags: warm, organic, intimate
2. Bass_Classic_MoogPunch [Bass, neutral] Tags: punchy, classic, powerful

Use 'generate output.json' to create synthesis configuration

> generate my_track.json
Generating synthesis configuration...
Generated synthesis configuration: my_track.json
Contains 2 instruments with full compatibility analysis

Configuration Validation:
Overall Score: 0.85 (RECOMMENDED)
- Semantic: 0.72
- Technical: 0.91
- Musical Role: 0.93
- Layering: 0.84
```

## Command Reference

### Search & Discovery
- `search <query>` - Semantic search with multi-dimensional ranking
- `boost <config_id>` - Mark configuration as preferred (future searches adapt)
- `demote <config_id>` - Mark configuration as disliked (future searches avoid)
- `exclude <config_id>` - Exclude from all future searches

### Selection & Management
- `select <config_id>` - Add to current selection with compatibility check
- `list` - Show selected configurations
- `stats` - System statistics and breakdown by role

### Generation & Export
- `generate [filename]` - Create synthesis-ready configuration (default: generated_config.json)
- `suggest_config [file]` - Alias for generate command

### Information & Help
- `help` - Command reference and usage tips
- `examples` - Workflow examples and patterns
- `quit` / `exit` - Exit application

## Scoring & Explainability

### Weighted Multi-Dimensional Scoring

```
Overall Score = 0.2 * semantic_score +      (20% semantic similarity)
                0.3 * technical_score +      (30% technical compatibility)
                0.3 * musical_role_score +   (30% musical role fitness)
                0.2 * layering_score         (20% arrangement suitability)
```

### Technical Compatibility Checks
- **Sample Rate**: Matching rates or convertible (44.1kHz, 48kHz, etc.)
- **Bit Depth**: 16/24/32-bit compatibility
- **Plugin Format**: VST/VST3/AU/AAX cross-compatibility
- **Envelope Types**: ADSR/DADSR/AHDSR compatibility
- **BPM Range**: Tempo overlap detection
- **Buffer Size**: Real-time processing compatibility
- **Polyphony**: Voice allocation validation

### Musical Role Compatibility Matrix

```
Lead     -> {Bass, Pad, Drums, Arp, Chord}
Bass     -> {Lead, Pad, Drums, Chord}
Pad      -> {Lead, Bass, Drums, Arp, Chord}
Arp      -> {Lead, Pad, Bass, Chord}
Drums    -> {Lead, Bass, Pad, Perc, Chord}
```

### Layering Analysis
- **Frequency Separation**: Low/Low-Mid/Mid/High-Mid/High/Full spectrum
- **Stereo Width**: Total width management (≤ 1.5 to avoid overcrowding)
- **Arrangement Layers**: 
  - Foreground (prominence > 0.7)
  - Midground (prominence 0.4-0.7)
  - Background (prominence < 0.4)
- **Mix Priority**: Balanced priority distribution

## Configuration & Tuning

### Weights Configuration

Edit `config/weights.json`:

```json
{
  "weights": {
    "semantic": 0.2,
    "technical": 0.3,
    "musicalRole": 0.3,
    "layering": 0.2
  },
  "thresholds": {
    "recommendationThreshold": 0.7,
    "technicalCompatibilityThreshold": 0.6
  }
}
```

## Build Targets

```bash
# Build targets
make              # Build release version (default)
make debug        # Build with debug symbols
make clean        # Remove build artifacts
make distclean    # Remove all generated files

# Setup & dependencies
make setup        # Download dependencies and setup directories
make create-sample-config  # Create sample configuration for testing

# Execution
make run          # Build and run the application
make run-with-config      # Run with specific configuration file
make test         # Run basic functionality tests

# Distribution
make install      # Install to system (Unix/Linux)
make uninstall    # Remove from system

# Development
make format       # Format source code (requires clang-format)
make analyze      # Run static analysis (requires cppcheck)
make docs         # Generate documentation (requires doxygen)
make help         # Show all available targets
```

## API Reference (C++)

### Core Classes

#### AudioConfig

```cpp
class AudioConfig {
public:
    AudioConfig(ConfigId id, std::string name, std::shared_ptr<nlohmann::json> configData);
    
    // Accessors
    const ConfigId& getId() const noexcept;
    const std::vector<std::string>& getSemanticTags() const noexcept;
    const EmbeddingVector& getEmbedding() const noexcept;
    const TechnicalSpecs& getTechnicalSpecs() const noexcept;
    const MusicalRoleInfo& getMusicalRole() const noexcept;
    const LayeringInfo& getLayeringInfo() const noexcept;
};
```

#### MultiDimensionalPointer

```cpp
class MultiDimensionalPointer {
public:
    MultiDimensionalPointer(ScoringWeights weights, std::shared_ptr<EmbeddingEngine> embeddingEngine);
    
    // Compatibility analysis
    CompatibilityResult analyzeCompatibility(
        const AudioConfig& configA, 
        const AudioConfig& configB) const;
    
    // Find compatible configurations
    std::vector<std::pair<std::shared_ptr<AudioConfig>, CompatibilityResult>> 
    findCompatibleConfigurations(
        const AudioConfig& anchor, 
        const std::vector<std::shared_ptr<AudioConfig>>& candidates, 
        const UserContext& userContext, 
        int maxResults = 10) const;
};
```

#### ConfigGenerator

```cpp
class ConfigGenerator {
public:
    // Generate synthesis-ready JSON
    static std::shared_ptr<nlohmann::json> generateSynthesisConfig(
        const std::vector<std::shared_ptr<AudioConfig>>& selectedConfigs,
        const UserContext& userContext);
    
    // Validate configuration chains
    static CompatibilityResult validateConfigChain(
        const std::vector<std::shared_ptr<AudioConfig>>& configChain);
};
```

## Testing & Validation

### Running Tests

```bash
# Build with debug information
make debug

# Run static analysis
make analyze

# Format code
make format

# Basic functionality tests
make test
```

### Performance Metrics (v1.2 Optimized)
- **Initialization**: <100ms for 30+ configurations
- **Search Response**: <1ms per query (~3x faster than v1.1)
- **Semantic Similarity**: O(d) with pre-normalized embeddings (~4x faster)
- **Tag Intersection**: O(min(m,n)) with hash sets (~10x faster)
- **4D Analysis**: <5ms per compatibility check
- **Memory Usage**: ~50MB for comprehensive system
- **All Scores**: Properly bounded to [0,1] range

## Integration Examples

### Synthesis System Integration

```cpp
// Load generated configuration
std::ifstream configFile("generated_config.json");
nlohmann::json synthesisConfig;
configFile >> synthesisConfig;

// Extract clean configuration data (no AI metadata)
for (const auto& [instrumentId, config] : synthesisConfig["instruments"].items()) {
    // Direct use in synthesis engine
    synthesizer.addInstrument(instrumentId, config);
}
```

### DAW Plugin Integration

```cpp
// Check plugin host compatibility
bool isCompatibleWithHost(const AudioConfig& config, const std::string& hostName) {
    const auto& pluginInfo = config.getTechnicalSpecs().pluginInfo;
    return std::find(pluginInfo.hostCompatibility.begin(), 
                    pluginInfo.hostCompatibility.end(), 
                    hostName) != pluginInfo.hostCompatibility.end();
}
```

## Platform Support

- **Linux**: Native support
- **macOS**: Native support
- **Windows**: MINGW32/MINGW64/MSYS2 support (see WINDOWS_BUILD.md)

## Professional Features

### Real-World Audio Compatibility
- VST/VST3/AU/AAX format validation
- Sample rate conversion planning
- Buffer size optimization
- Latency calculation and compensation
- CPU usage estimation and balancing

### Music Production Best Practices
- Frequency masking avoidance
- Stereo field optimization
- Dynamic range balancing
- Arrangement context awareness
- Genre-specific recommendations

### Workflow Support
- Template generation for different genres
- Progressive enhancement (add complementary instruments)
- Conflict resolution with specific suggestions
- Batch processing for multiple projects

## Development & Contribution

### Code Style
- **Modern C++17**: RAII, smart pointers, type safety
- **Audio Terminology**: Clear naming with domain-specific terms
- **Doxygen Documentation**: Complete API documentation
- **Exception Safety**: Strong exception guarantees

### Architecture Principles
- **Single Responsibility**: Each class has a focused purpose
- **Dependency Injection**: Testable and configurable
- **Interface Segregation**: Minimal, focused interfaces
- **Open/Closed**: Extensible without modification

## License & Credits

This Multi-Dimensional Audio Configuration System demonstrates modern C++17 development practices for professional audio applications, combining AI-driven analysis with real-world technical constraints.

**Built with:**
- nlohmann/json - Modern JSON for C++
- FastText-style embeddings - Semantic understanding
- Professional audio standards - Real-world compatibility

---

*Ready for integration with professional audio production workflows and synthesis systems.*
