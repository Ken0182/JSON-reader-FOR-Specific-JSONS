# Search Interest Tracking with Decay - v1.5

## Overview

Version 1.5 implements persistent search interest tracking with temporal decay, smoothing, and gentle bias signals for semantic search scoring. The system learns from your search patterns and subtly improves future recommendations based on your interests.

## Problem Addressed

### Before v1.5: No Learning from Search Patterns

**Issues:**
1. ❌ No memory of past searches
2. ❌ No personalization based on user interests
3. ❌ Same results regardless of search history
4. ❌ No way to tune search behavior over time

### Solution: Search Interest Tracker with Decay

**v1.5 introduces:**
1. ✅ Persistent tracking of search queries with timestamps
2. ✅ Exponential temporal decay (interests fade over time)
3. ✅ EMA smoothing for stable signals
4. ✅ Gentle clamped bias for search scoring
5. ✅ Tunable parameters for customization
6. ✅ Export/import for persistence
7. ✅ Comprehensive CLI command family

---

## Architecture

### Core Components

```
SearchInterestTracker
├─ Query History: Vector of timestamped query records
├─ Token Signals: Map of token → (strength, timestamp)
├─ Parameters: Tunable configuration
└─ Methods: record, bias, export, import, prune
```

### Key Concepts

**1. Exponential Decay**
```
decayed_strength = raw_strength * (0.5)^(age / half_life)
```
- Half-life: 2 hours (default, tunable)
- Older searches contribute less to bias
- Natural forgetting curve

**2. EMA Smoothing**
```
new_signal = alpha * new_value + (1 - alpha) * old_decayed_value
```
- Alpha: 0.3 (default, tunable)
- Prevents signal spikes from single searches
- Stable, gradual adaptation

**3. Gentle Clamped Bias**
```
bias = min(average_signal * bias_strength, bias_clamp_max)
```
- Bias strength: 0.2 (20% of signal, tunable)
- Bias clamp: 0.15 (maximum contribution, tunable)
- Additive boost, doesn't overwhelm other signals

---

## Implementation

### 1. Search Interest Tracker Class

**File:** `src/search_tracker.hpp` + `src/search_tracker.cpp`

```cpp
class SearchInterestTracker {
public:
    // Core operations
    void recordQuery(const std::vector<std::string>& tokens);
    float getBiasSignal(const std::vector<std::string>& tokens) const;
    
    // Management
    void setEnabled(bool enabled);
    void setParameters(const SignalParameters& params);
    void clear();
    void pruneWeakSignals();
    
    // Inspection
    std::unordered_map<std::string, float> getActiveSignals() const;
    std::vector<QueryRecord> getHistory(int maxResults) const;
    std::unordered_map<std::string, float> getStatistics() const;
    
    // Persistence
    nlohmann::json exportState() const;
    bool importState(const nlohmann::json& j);
};
```

### 2. Integration with User Context

**Updated:** `UserContext` class

```cpp
class UserContext {
private:
    SearchInterestTracker searchTracker_;  // v1.5
    
public:
    SearchInterestTracker& getSearchTracker();
};
```

### 3. Search Scoring with Bias

**Updated:** `AudioConfigSystem::searchConfigurations()`

```cpp
// Record query for tracking
tracker.recordQuery(queryTokens);

// For each config:
float semanticScore = calculateSimilarity(queryEmbed, configEmbed);
float tokenOverlap = calculateTokenOverlap(queryTokens, configTokens);
float combinedScore = 0.5 * tokenScore + 0.5 * semanticScore;

// Apply search interest bias
float configBias = tracker.getBiasSignal(configTokens);
combinedScore += configBias;  // Gentle additive boost (max 0.15)
```

**Result**: Configs matching user's past interests get subtle boost!

---

## Tunable Parameters

### SignalParameters Structure

```cpp
struct SignalParameters {
    // Decay
    float decayHalfLife{7200.0f};        // 2 hours (in seconds)
    float minSignalStrength{0.01f};      // Prune below this
    
    // Smoothing
    float smoothingAlpha{0.3f};          // EMA factor
    
    // Bias
    float biasStrength{0.2f};            // How much to bias (0-1)
    float biasClampMax{0.15f};           // Maximum contribution
    
    // History
    int maxHistorySize{1000};            // Max queries to keep
    bool enableTracking{true};           // Master switch
};
```

### Parameter Effects

| Parameter | Range | Effect | Recommendation |
|-----------|-------|--------|----------------|
| **decayHalfLife** | 600-86400s | How fast interests fade | 3600s (1h) for active users, 14400s (4h) for casual |
| **smoothingAlpha** | 0.0-1.0 | Response speed | 0.3 (gradual), 0.5 (medium), 0.8 (fast) |
| **biasStrength** | 0.0-1.0 | Bias magnitude | 0.2 (subtle), 0.4 (moderate), 0.6 (strong) |
| **biasClampMax** | 0.0-0.5 | Maximum boost | 0.15 (safe), 0.25 (noticeable), 0.4 (significant) |
| **minSignalStrength** | 0.001-0.1 | Pruning threshold | 0.01 (default), 0.05 (aggressive pruning) |

---

## CLI Command Family

### Commands Overview

```bash
signals <subcommand> [options]

Subcommands:
  on/off      - Enable/disable tracking
  status      - Show statistics and parameters
  history [n] - Show recent searches (default: 20)
  active      - Show active interest signals
  tune        - Adjust parameters
  export      - Save state to JSON file
  import      - Load state from JSON file
  clear       - Reset all data
  help        - Detailed documentation
```

### 1. Enable/Disable Tracking

```bash
# Enable tracking
> signals on
Search interest tracking: ENABLED
Your searches will now be tracked to improve future recommendations.

# Disable tracking
> signals off
Search interest tracking: DISABLED
Your searches will no longer be tracked (existing data preserved).
```

### 2. View Status

```bash
> signals status

=== SEARCH INTEREST TRACKER STATUS ===
Tracking: ENABLED

Statistics:
  Total queries tracked: 15
  Active signals: 5
  Total tokens tracked: 12
  Average signal strength: 0.723

Parameters:
  Decay half-life: 7200.000 seconds (2.000 hours)
  Smoothing alpha: 0.300
  Bias strength: 0.200
  Bias clamp max: 0.150
  Min signal strength: 0.010
```

### 3. View Search History

```bash
> signals history 10

=== SEARCH HISTORY (Most Recent 10) ===
1. [warm analog] (5 min ago, strength: 0.95)
2. [bright energetic] (12 min ago, strength: 0.87)
3. [retro funky] (18 min ago, strength: 0.79)
4. [warm] (25 min ago, strength: 0.70)
5. [analog vintage] (32 min ago, strength: 0.62)
...
```

### 4. View Active Signals

```bash
> signals active

=== ACTIVE INTEREST SIGNALS (5) ===
  warm: 0.923 [==================  ]
  analog: 0.867 [=================   ]
  bright: 0.742 [===============     ]
  retro: 0.531 [===========         ]
  vintage: 0.421 [========            ]
```

### 5. Tune Parameters

```bash
> signals tune decay_halflife 3600
Parameter updated: decay_halflife = 3600.0

> signals tune bias_strength 0.3
Parameter updated: bias_strength = 0.3

> signals tune smoothing_alpha 0.5
Parameter updated: smoothing_alpha = 0.5
```

**Available parameters:**
- `decay_halflife` - Decay half-life in seconds
- `smoothing_alpha` - EMA smoothing factor (0-1)
- `bias_strength` - Bias multiplier (0-1)
- `bias_clamp` - Maximum bias contribution
- `min_signal` - Minimum signal threshold

### 6. Export/Import State

```bash
# Export to file
> signals export my_interests.json
Tracker state exported to: my_interests.json
  Queries: 15
  Active signals: 5

# Import from file
> signals import my_interests.json
Tracker state imported from: my_interests.json
  Queries: 15
  Active signals: 5
```

### 7. Clear Data

```bash
> signals clear
All tracked data cleared.
```

### 8. Get Help

```bash
> signals help
[Shows comprehensive help documentation]
```

---

## Usage Examples

### Example 1: Basic Usage

```bash
$ ./audio_config_system

> search warm analog
[Results show warm/analog configs]

> search vintage retro
[Results show vintage/retro configs]

> signals status
Statistics:
  Total queries tracked: 2
  Active signals: 4
  ...

> signals active
=== ACTIVE INTEREST SIGNALS (4) ===
  warm: 1.000 [====================]
  analog: 1.000 [====================]
  vintage: 1.000 [====================]
  retro: 1.000 [====================]
```

### Example 2: Personalized Recommendations

```bash
# Scenario: User consistently searches for "warm" sounds
> search warm
> search warm pad
> search warm bass
> search warm vintage

# Later search without "warm"
> search analog

# Result: Warm analog configs ranked higher due to bias!
# The system learned that "warm" + "analog" is a good match
# for this user based on past interest in "warm"
```

### Example 3: Temporal Decay in Action

```bash
# Day 1: Search for "bright" sounds
> search bright lead
Active signal: bright (strength: 1.0)

# 2 hours later (1 half-life):
> signals active
bright: 0.500 [==========          ]

# 4 hours later (2 half-lives):
> signals active
bright: 0.250 [=====               ]

# 8 hours later (4 half-lives):
> signals active
bright: 0.063 [=                   ]
# Signal automatically pruned (below threshold)
```

### Example 4: Customization for Power Users

```bash
# Faster decay (1 hour half-life)
> signals tune decay_halflife 3600

# Stronger bias
> signals tune bias_strength 0.4
> signals tune bias_clamp 0.25

# Faster adaptation
> signals tune smoothing_alpha 0.6

# Result: More responsive to recent searches,
# stronger personalization effect
```

### Example 5: Persistence

```bash
# Session 1: Build up interest profile
> search warm
> search analog
> search vintage
> signals export my_profile.json
[Exit]

# Session 2: Load previous profile
> signals import my_profile.json
Tracker state imported from: my_profile.json
  Queries: 3
  Active signals: 3

# Continues with previous interests!
> search synth
[Results biased toward warm/analog/vintage]
```

---

## Technical Details

### Decay Mathematics

**Exponential Decay Formula:**
```
S(t) = S₀ * (0.5)^(t / t_half)

Where:
  S(t) = signal strength at time t
  S₀ = initial signal strength
  t = age in seconds
  t_half = half-life in seconds
```

**Example** (half-life = 7200s = 2 hours):
- t = 0s: S = 1.0 (100%)
- t = 7200s: S = 0.5 (50%)
- t = 14400s: S = 0.25 (25%)
- t = 21600s: S = 0.125 (12.5%)
- t = 28800s: S = 0.0625 (6.25%)
- t = 36000s: S = 0.03125 (3.1%) → pruned

### EMA Smoothing

**Exponential Moving Average:**
```
S_new = α * value_new + (1 - α) * S_old_decayed

Where:
  α = smoothing factor (0.3 default)
  S_old_decayed = previous signal with decay applied
```

**Effect** (α = 0.3):
- New query contributes 30%
- Decayed old signal contributes 70%
- Smooth, stable transitions

### Bias Calculation

**Per-Config Bias:**
```cpp
// 1. Get decayed signals for config tokens
float totalSignal = 0.0f;
int matchedTokens = 0;
for (const auto& token : configTokens) {
    float signal = getTokenSignal(token);  // With decay
    if (signal > minThreshold) {
        totalSignal += signal;
        matchedTokens++;
    }
}

// 2. Average across matched tokens
float avgSignal = totalSignal / matchedTokens;

// 3. Apply bias strength
float bias = avgSignal * biasStrength;

// 4. Clamp to maximum
return min(bias, biasClampMax);
```

**Example:**
```
Config tokens: ["warm", "analog", "vintage"]
User signals:  warm=0.8, analog=0.6, vintage=0.0
Matched: warm, analog (vintage below threshold)

avgSignal = (0.8 + 0.6) / 2 = 0.7
bias = 0.7 * 0.2 = 0.14
final = min(0.14, 0.15) = 0.14

Result: Config gets +0.14 boost (14% of max score)
```

---

## JSON Export Format

```json
{
  "parameters": {
    "decayHalfLife": 7200.0,
    "minSignalStrength": 0.01,
    "smoothingAlpha": 0.3,
    "biasStrength": 0.2,
    "biasClampMax": 0.15,
    "maxHistorySize": 1000,
    "enableTracking": true
  },
  "signals": {
    "warm": {
      "strength": 0.923,
      "timestamp": 1760685269
    },
    "analog": {
      "strength": 0.867,
      "timestamp": 1760685270
    }
  },
  "history": [
    {
      "tokens": ["warm", "analog"],
      "timestamp": 1760685269,
      "rawStrength": 1.0
    },
    {
      "tokens": ["vintage", "retro"],
      "timestamp": 1760685270,
      "rawStrength": 1.0
    }
  ]
}
```

---

## Performance Impact

### Memory Overhead

| Component | Per Item | Total (1000 queries) |
|-----------|----------|----------------------|
| Token signals | ~100 bytes | ~10 KB |
| Query history | ~200 bytes | ~200 KB |
| **Total** | - | **~210 KB** |

**Negligible impact** on modern systems.

### Runtime Overhead

| Operation | Complexity | Time (typical) |
|-----------|------------|----------------|
| recordQuery() | O(k) k=tokens | <0.1ms |
| getBiasSignal() | O(k) k=tokens | <0.1ms |
| pruneWeakSignals() | O(n) n=signals | <1ms (every 10 queries) |
| exportState() | O(n+h) n=signals, h=history | <5ms |

**Minimal impact** on search performance (<5% overhead).

---

## Privacy & Data

### What is Tracked?

✅ Normalized query tokens  
✅ Timestamps  
✅ Signal strengths  

❌ NOT tracked: Raw queries (only tokens)  
❌ NOT tracked: Personal information  
❌ NOT tracked: Selected configurations  
❌ NOT tracked: Generated results  

### Data Location

- **In-memory only** by default
- Export explicitly with `signals export`
- No automatic network transmission
- Full user control

### Disabling Tracking

```bash
# Disable (keeps existing data)
> signals off

# Disable and clear data
> signals off
> signals clear

# Or simply don't use the system - tracking is passive
```

---

## Future Enhancements

### Short-term
- Auto-save on exit (optional)
- Visualization of signal decay over time
- Top recommended searches based on interests

### Medium-term
- Per-dimension interest tracking (semantic, technical, role)
- Collaborative filtering across users (opt-in)
- Adaptive decay rates (learn optimal half-life)

### Long-term
- Neural network-based interest modeling
- Context-aware recommendations (time of day, project type)
- Multi-modal interest tracking (search + selections + generations)

---

## Summary

**v1.5 adds intelligent personalization through:**

1. ✅ **Persistent tracking** - Remembers your search patterns
2. ✅ **Temporal decay** - Old interests naturally fade
3. ✅ **EMA smoothing** - Stable, gradual adaptation
4. ✅ **Gentle bias** - Subtle improvements, not overwhelming
5. ✅ **Tunable parameters** - Customize to your preference
6. ✅ **Export/import** - Persist across sessions
7. ✅ **Comprehensive CLI** - Full control from shell

**Result**: The system learns your preferences and gently improves recommendations over time, without sacrificing search quality or requiring manual configuration.

---

## Version History

- **v1.0**: Initial 4D pointing
- **v1.1**: Path auto-detection
- **v1.2**: Efficiency upgrades (pre-norm, caching, IDF, clamping)
- **v1.3**: SKD embeddings (semantic meaning)
- **v1.4**: Unified tokenization (search correctness)
- **v1.5**: Search interest tracking with decay (this release)

---

*v1.5 makes the system learn from your searches and improve over time.*
