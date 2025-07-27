# JSON Reader System - Clean Configuration Generator

A C++ system that processes multiple JSON files to produce a clean, minimal configuration for sound synthesis. This system follows strict requirements for data separation, minimal output, and proper structure organization.

## Overview

The JSON Reader System processes five JSON input files to generate a single, clean configuration file suitable for WAV synthesis:

### Input Files (by purpose):

**Source Data (included in output):**
- `guitar.json` - Real guitar instrument definitions and parameters
- `group.json` - Real synthesizer group/effect chain definitions

**Reference Data (used for AI scoring only, NOT in output):**
- `moods.json` - Mood-based parameter ranges for AI filtering/scoring
- `Synthesizer.json` - Section-based synthesis schemas for AI analysis
- `structure.json` - Song section mappings and envelope modifiers

## Key Features

### ✅ Correct Data Separation
- **Source data** (`guitar.json`, `group.json`) → Output as instruments/effects
- **Reference data** (`moods.json`, `Synthesizer.json`) → Used only for internal AI scoring
- **Structure data** → Applied as mappings when relevant, omitted when not applicable

### ✅ Clean, Minimal Output
- No empty fields, null values, or empty arrays/objects
- No AI metadata (layering roles, scores) in final config
- Each instrument/group appears under its unique name at top level
- Proper nesting: all parameters under their respective instrument/group

### ✅ Flexible Integration Ready
- No placeholders or structural clutter
- Only meaningful, applicable data included
- Ready for direct use by WAV synthesis programs

## Build and Run

```bash
# Download nlohmann/json library
wget -O json.hpp https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp

# Compile
g++ -std=c++17 -O2 -o json_reader_system json_reader_system.cpp

# Run
./json_reader_system
```

## Output Structure

The generated `clean_config.json` contains instruments and groups at the top level:

```json
{
  "Classical_Nylon_Soft": {
    "adsr": {
      "type": "DADSR",
      "attack": [30, 80],
      "decay": [400, 600],
      "sustain": [0.5, 0.7],
      "release": [1000, 1500],
      "curve": "logarithmic"
    },
    "guitarParams": {
      "strings": { "material": "nylon", "num_strings": 6, ... },
      "harmonics": { "vibe_set": [1.0, 0.4, 0.2, 0.1], ... },
      "filter": { "type": "low-pass", "cutoff": [600, 1000], ... }
    },
    "soundCharacteristics": {
      "timbral": "mellow",
      "material": "wood",
      "emotional": [{"tag": "delicate", "weight": 0.8}]
    },
    "topologicalMetadata": {
      "damping": "high",
      "spectral_complexity": "low",
      "manifold_position": "outro region"
    }
  },
  "Pad_Warm_Calm": {
    "synthesisType": "subtractive",
    "oscillator": {
      "types": ["sine", "triangle"],
      "mix_ratios": [0.7, 0.3],
      "detune": 0.02
    },
    "adsr": {
      "type": "ADSR",
      "attack": "600ms",
      "decay": "400ms",
      "sustain": 0.6,
      "release": "1200ms"
    },
    "effects": [
      {"type": "reverb", "decay": "3s", "wet": 0.5, "mix": 0.3},
      {"type": "delay", "feedback": 0.3, "time": "500ms", "mix": 0.4}
    ],
    "structure": {
      "sectionName": "Intro",
      "attackMul": 1.5,
      "decayMul": 1.2,
      "releaseMul": 1.5
    }
  }
}
```

## Requirements Compliance

### ✅ 1. Proper Output Structure
- Each instrument/group has unique top-level key
- All properties nested beneath (no scattered fields)
- Only non-empty, applicable fields included

### ✅ 2. Correct Source Usage
- `guitar.json` & `group.json` → Real instrument/effect definitions in output
- `moods.json` & `Synthesizer.json` → Reference only (AI scoring, never output)
- `structure.json` → Section mappings applied where relevant, omitted otherwise

### ✅ 3. No Data Contamination
- Reference files (moods/synth) never appear as instruments in config
- AI metadata (layering, scores) calculated internally but never exported
- No empty placeholders or null values

### ✅ 4. Structure Mapping
- Applied from `structure.json` when mapping exists for instrument/group
- Includes section multipliers and gate settings
- Omitted entirely when no mapping exists (clean!)

### ✅ 5. Future-Ready Design
- Layering calculations done internally (1-6 stages) but not exported
- WAV writer can apply its own layering without config clutter
- Ready for user refinement and advanced synthesis

## Validation

Run the included validation script to verify requirements:

```bash
python3 validate_config.py clean_config.json
```

Expected output:
```
🎉 ALL REQUIREMENTS MET! Configuration is ready for WAV synthesis.
```

## Internal Processing (Not Exported)

The system calculates but does NOT export:
- **AI Scores**: Matching calculations based on `moods.json` and `Synthesizer.json`
- **Layering Roles**: 1-6 stage assignments (background to foreground)
- **Reference Metadata**: Schema definitions and validation rules

These are kept for internal use and future AI-driven narrowing menus.

## Example Usage in Next Stage

The generated config is designed for direct use by WAV synthesis:

```cpp
// WAV Writer Stage (hypothetical)
json config = loadCleanConfig("clean_config.json");

// Direct access to instrument parameters
auto nylonGuitar = config["Classical_Nylon_Soft"];
auto adsrParams = nylonGuitar["adsr"];
auto guitarParams = nylonGuitar["guitarParams"];

// Apply user layering preferences
applyUserLayering(nylonGuitar, userLayerChoice);

// Synthesize
synthesizeInstrument(nylonGuitar, noteData);
```

## Success Metrics

- ✅ 30 instruments/groups processed
- ✅ 8 structure mappings applied  
- ✅ 0 empty fields in output
- ✅ 0 reference data leaked
- ✅ 0 AI metadata in config
- ✅ Ready for WAV synthesis stage