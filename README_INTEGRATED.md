# Integrated AI Synthesis System

A comprehensive AI-driven instrument synthesis system that integrates FastText embeddings with a pointing index system for intelligent configuration search, selection, and management.

## Overview

This system combines advanced NLP embeddings with instrument configuration management to provide:

- **FastText Embedding Engine**: Subword-aware embeddings for robust semantic understanding
- **Enhanced Semantic Database**: Rich keyword database with categories and weighted scoring
- **Pointing Index System**: Intelligent configuration indexing with user feedback integration
- **Interactive Command Interface**: Full-featured CLI for search, selection, and management
- **Dynamic Configuration Export**: Real-time configuration export with user modifications

## Features

### ✨ FastText Embedding Engine
- **Subword modeling** for handling unknown words and technical audio terms
- **Sentence-level embeddings** for comprehensive text understanding
- **Audio vocabulary initialization** with pre-trained music/audio terminology
- **Similarity computation** using cosine similarity for semantic matching
- **Embedding persistence** with save/load functionality

### 🎯 Enhanced Semantic Database
- **Categorized keywords** (timbral, emotional, dynamic, material, structural)
- **Weighted scoring** with customizable importance factors
- **Alias support** for synonym matching and expanded vocabulary
- **FastText integration** for deep semantic understanding
- **Extensible design** for adding new categories and keywords

### 🔍 Pointing Index System
- **Intelligent search** using embedding-based similarity
- **User feedback integration** with boost/demote functionality
- **Configuration exclusion** for filtering unwanted results
- **Semantic expansion** for enhanced query understanding
- **Real-time ranking** with personalized user preferences

### 💻 Interactive Command Interface
- **Search operations** with natural language queries
- **Configuration management** with detailed parameter display
- **User preference learning** through boost/demote commands
- **Statistics and analytics** for system monitoring
- **Export functionality** for configuration persistence

## Installation

### Prerequisites
- C++17 compatible compiler (GCC 7+, Clang 5+)
- Make utility
- Standard C++ library

### Building from Source

```bash
# Clone or download the source
git clone <repository-url>
cd integrated-ai-synthesis

# Build the system
make

# Run the program
make run

# Or run directly
./ai_synthesis
```

### Build Options

```bash
# Debug build with full symbols
make debug

# Optimized release build
make release

# Install to system path
make install

# Clean build artifacts
make clean
```

## Usage

### Starting the System

```bash
# Start interactive mode
./ai_synthesis
```

You'll see:
```
AI Synthesis System v2.0
Integrated FastText Embedding Engine with Pointing Index
============================================================
Initializing system...
Initialized semantic database with 18 entries
Initialized 8 default configurations
Rebuilt index for 8 configurations
System ready!
Type 'help' for available commands.

ai_synthesis> 
```

### Available Commands

#### Search Operations
```bash
# Basic search
search warm guitar

# Semantic expansion search (uses keyword database)
search_semantic ambient dreamy

# Find similar words to a term
similar bright

# Find semantic database matches
semantic aggressive powerful
```

#### Configuration Management
```bash
# List all configurations
list

# Select and display a configuration
select guitar_warm_acoustic

# View system statistics
stats
```

#### User Feedback & Learning
```bash
# Boost a configuration (increase ranking)
boost guitar_warm_acoustic 0.2

# Demote a configuration (decrease ranking)
demote synth_aggressive_lead 0.1

# Exclude from search results
exclude drums_electronic_kit

# Include back in search results
include drums_electronic_kit
```

#### Data Export
```bash
# Export current configuration state
export my_config.json

# Export with custom filename
export custom_name.json
```

### Example Workflow

1. **Search for configurations:**
   ```bash
   ai_synthesis> search warm acoustic guitar
   ```

2. **Review results and select one:**
   ```bash
   ai_synthesis> select guitar_warm_acoustic
   ```

3. **Adjust preferences:**
   ```bash
   ai_synthesis> boost guitar_warm_acoustic 0.3
   ai_synthesis> demote guitar_bright_electric 0.1
   ```

4. **Search again to see improved results:**
   ```bash
   ai_synthesis> search warm guitar
   ```

5. **Export your personalized configuration:**
   ```bash
   ai_synthesis> export my_preferences.json
   ```

## System Architecture

### Core Components

```
┌─────────────────────┐    ┌─────────────────────┐
│  FastText Engine    │    │  Semantic Database  │
│  - Word embeddings  │    │  - Categorized KWs  │
│  - Subword modeling │    │  - Weighted scoring │
│  - Sentence vectors │    │  - Alias support    │
└─────────────────────┘    └─────────────────────┘
           │                           │
           └─────────┬─────────────────┘
                     │
           ┌─────────▼─────────┐
           │  Pointing Index   │
           │  - Configuration  │
           │  - User feedback  │
           │  - Search & rank  │
           └─────────┬─────────┘
                     │
           ┌─────────▼─────────┐
           │  Command Line     │
           │  - Interactive    │
           │  - Search/Select  │
           │  - Export/Stats   │
           └───────────────────┘
```

### Data Flow

1. **Initialization**: System loads default configurations and builds embeddings
2. **User Query**: Natural language search query entered
3. **Embedding Generation**: Query converted to FastText embedding vector
4. **Semantic Expansion**: Optional expansion using semantic database
5. **Similarity Computation**: Cosine similarity calculated for all configurations
6. **Ranking & Filtering**: Results ranked with user feedback applied
7. **Display**: Top results shown with detailed information
8. **User Feedback**: Boost/demote commands update future rankings
9. **Export**: Final configuration state saved to JSON

## Configuration Format

### Default Configurations

The system includes these default instrument configurations:

- **guitar_warm_acoustic**: Warm acoustic guitar with natural wood character
- **guitar_bright_electric**: Bright electric guitar with crisp highs
- **synth_lush_pad**: Rich ambient pad with multiple oscillators
- **synth_aggressive_lead**: Cutting lead synthesizer with aggressive filtering
- **bass_deep_sub**: Deep sub bass with long sustain
- **bass_punchy_electric**: Punchy electric bass with percussive attack
- **drums_acoustic_kit**: Natural acoustic drums with room ambience
- **drums_electronic_kit**: Synthetic electronic drums with effects

### Configuration Structure

```json
{
  "id": "guitar_warm_acoustic",
  "name": "Warm Acoustic Guitar",
  "instrument_type": "acoustic_guitar",
  "description": "Warm, resonant acoustic guitar with natural wood character",
  "tags": ["warm", "acoustic", "organic"],
  "parameters": {
    "attack": {
      "value": "0.01",
      "description": "Fast attack for plucked strings"
    },
    "decay": {
      "value": "1.2", 
      "description": "Natural decay time"
    }
  }
}
```

### Semantic Database

Keywords are organized by category:

- **Timbral**: warm, bright, dark, lush, gritty, ethereal
- **Emotional**: calm, energetic, nostalgic, aggressive, dreamy
- **Dynamic**: punchy, smooth, percussive
- **Material**: organic, synthetic, metallic
- **Structural**: intro, verse, chorus

Each keyword includes:
- Primary term and aliases
- Category classification
- Score weight
- FastText embedding

## Advanced Features

### FastText Embeddings

The system uses subword-aware embeddings that can handle:
- **Unknown words** through character n-gram modeling
- **Technical terms** specific to audio synthesis
- **Morphological variants** (e.g., "synthesize" → "synthesizer")
- **Semantic similarity** between related concepts

### User Learning

The system learns from user interactions:
- **Boost commands** increase configuration scores permanently
- **Demote commands** decrease scores for unwanted results
- **Exclusion filters** remove configurations from future searches
- **Preference persistence** through export/import functionality

### Semantic Expansion

Search queries can be automatically expanded:
```bash
# Query: "ambient dreamy"
# Expanded to: "ambient dreamy ethereal floating surreal calm peaceful"
```

This improves recall by including semantically related terms.

### Real-time Analytics

The system provides comprehensive statistics:
- Configuration counts by instrument type
- User boost/demote statistics
- Embedding dimensions and vocabulary size
- Search history and result counts

## Extending the System

### Adding New Configurations

```cpp
auto newConfig = std::make_unique<ConfigurationEntry>(
    "new_id",
    "New Configuration",
    "instrument_type",
    "Description of the new configuration"
);
newConfig->addTag("custom_tag");
newConfig->addParameter("param_name", "param_value", "description");

indexSystem.addConfiguration(std::move(newConfig));
```

### Adding Semantic Keywords

```cpp
semanticDb.addEntry(
    "new_keyword",
    "category",
    {"alias1", "alias2"},
    0.9f,  // score weight
    "Description of the keyword"
);
```

### Custom Embedding Training

```cpp
std::vector<std::string> corpus = {
    "Custom training text for domain-specific terms",
    "Additional audio synthesis vocabulary",
    // ... more training data
};

embeddingEngine.trainOnCorpus(corpus);
```

## Performance Characteristics

- **Embedding dimension**: 100 (configurable)
- **Vocabulary size**: ~50 pre-loaded terms, grows dynamically
- **Search latency**: < 10ms for typical queries
- **Memory usage**: ~5MB for default configuration
- **Scalability**: Linear with number of configurations

## Troubleshooting

### Common Issues

**Build Errors:**
```bash
# Ensure C++17 support
g++ --version  # Should be 7.0+

# Check for missing headers
apt-get install build-essential  # Ubuntu/Debian
```

**Runtime Issues:**
```bash
# Permission errors on export
chmod +w .  # Ensure write permissions

# Memory issues
ulimit -v 1000000  # Set memory limit
```

**Search Not Working:**
- Check that configurations were loaded properly
- Verify embedding generation completed
- Try simpler search terms initially

### Debug Mode

```bash
# Build with debug symbols
make debug

# Run with verbose output
gdb ./ai_synthesis
```

### Performance Tuning

```bash
# Release build for optimal performance
make release

# Profile with gprof
g++ -pg integrated_ai_synthesis_system.cpp -o ai_synthesis
./ai_synthesis
gprof ai_synthesis gmon.out > profile.txt
```

## API Reference

### FastTextEmbeddingEngine
- `getSentenceEmbedding(text)`: Generate embedding for text
- `computeTextSimilarity(text1, text2)`: Compute similarity score
- `findSimilarWords(word, k)`: Find k most similar words
- `trainOnCorpus(documents)`: Train on document collection

### EnhancedSemanticDatabase
- `findMatches(query, k, threshold)`: Find semantic matches
- `addEntry(keyword, category, aliases, weight, desc)`: Add new keyword
- `getEntry(keyword)`: Retrieve keyword entry
- `getAllCategories()`: Get all categories

### PointingIndexSystem
- `search(query, maxResults, threshold)`: Search configurations
- `searchWithSemanticExpansion(query)`: Search with expansion
- `boostConfiguration(id, amount)`: Boost ranking
- `demoteConfiguration(id, amount)`: Demote ranking
- `excludeConfiguration(id, exclude)`: Exclude/include configuration

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Submit a pull request

### Code Style
- Follow C++17 best practices
- Use descriptive variable and function names
- Add comprehensive documentation
- Include unit tests for new features

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- FastText methodology inspired by Facebook's FastText
- Semantic database design based on WordNet principles
- User feedback integration following collaborative filtering approaches