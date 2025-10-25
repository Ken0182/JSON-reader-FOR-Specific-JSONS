/**
 * @file audio_config_system.hpp
 * @brief Multi-Dimensional Audio Configuration System - Main Header
 * @author AI Assistant
 * @version 1.6
 * 
 * v1.6 Semantic Knowledge Base (MAJOR UPGRADE):
 * - SQLite database for tags, embeddings, aliases, IDF stats
 * - Dynamic embedding dimensions (no hardcoded 100D limit)
 * - Sentence encoder for unlimited vocabulary (any user text)
 * - Contrastive query vectors (positive - negative constraints)
 * - Explainability (top contributing tags in results)
 * - Config-driven tuning (weights stored in database)
 * - Meaning-aware search (free-text intent, not brittle rules)
 * 
 * v1.5 Search Interest Tracking:
 * - Persistent user search patterns with temporal decay
 * - EMA smoothing for stable signal adaptation
 * - Gentle clamped bias for personalization
 * 
 * v1.4 Unified Tokenization & Scoring:
 * - Shared tokenization pipeline (camelCase, snake_case, diacritics, punctuation)
 * - Per-token matching across IDs, tags, queries
 * - Aligned embedding generation (uses same token stream)
 * - Re-ranking with cosine-on-shared-tokens validation
 * 
 * v1.3 SKD Embedding Integration:
 * - External Semantic Knowledge Database (SKD) embedding index
 * - Semantically meaningful vectors (replaces hash-based embeddings)
 * - Richer vocabulary with real pre-trained embeddings
 * - Normalized at build time for fast dot-product similarity
 * 
 * v1.2 Efficiency & Quality Upgrades:
 * - Pre-normalized embeddings for faster similarity calculation
 * - Cached tag sets for O(1) intersection (vs O(m·n) nested loops)
 * - IDF-weighted tag boost with λ clamping
 * - Proper score clamping to [0,1] range
 * - Optional diagonal weighting for semantic dimensions
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <functional>
#include <cmath>
#include "json.hpp"
#include "text_utils.hpp"
#include "search_tracker.hpp"
#include "semantic_knowledge_base.hpp"  // v1.6: Only new include needed

namespace audio_config {

// Type aliases for clarity
using ConfigId = std::string;
using EmbeddingVector = std::vector<float>;  // DYNAMIC DIMENSION (PRE-NORMALIZED)
using ScoreWeight = float;
using CompatibilityScore = float;
using TagSet = std::unordered_set<std::string>;  // O(1) lookup for tag intersection

/**
 * @brief Audio plugin format enumeration
 */
enum class PluginFormat {
    VST2,
    VST3,
    AU,      // Audio Units (macOS)
    AAX,     // Pro Tools
    CLAP,    // CLever Audio Plugin
    Unknown
};

/**
 * @brief Musical role classification
 */
enum class MusicalRole {
    Lead,        // Primary melodic instrument
    Bass,        // Foundation/low-end
    Pad,         // Harmonic texture/background
    Arp,         // Arpeggiated/rhythmic melodic
    Percussion,  // Rhythmic elements
    FX,          // Effects/processing
    Chord,       // Harmonic/chordal
    Unknown
};

/**
 * @brief Arrangement layer classification
 */
enum class ArrangementLayer {
    Foreground,  // Primary focus (leads, solos)
    Midground,   // Supporting elements (rhythm, arp)
    Background,  // Texture/ambience (pads, reverb)
    Unknown
};

/**
 * @brief Technical specifications for audio compatibility
 */
struct TechnicalSpecs {
    float sampleRate{44100.0f};
    int bitDepth{24};
    int polyphony{16};
    std::string envelopeType{"ADSR"};
    PluginFormat pluginFormat{PluginFormat::VST3};
    std::vector<std::string> supportedHosts;
    std::pair<float, float> bpmRange{60.0f, 200.0f};
    std::pair<int, int> bufferSizeRange{64, 2048};
    bool supportsMPE{false};  // MIDI Polyphonic Expression
    int latencyMs{0};
    std::string cpuUsage{"low"};  // low, medium, high
    
    /**
     * @brief Check if technical specs are compatible
     * @param other Other technical specifications
     * @return Compatibility score (0.0-1.0)
     */
    [[nodiscard]] CompatibilityScore isCompatibleWith(const TechnicalSpecs& other) const noexcept;
};

/**
 * @brief Musical role information
 */
struct MusicalRoleInfo {
    MusicalRole primaryRole{MusicalRole::Unknown};
    std::vector<MusicalRole> secondaryRoles;
    std::string musicalContext{"any"};  // intro, verse, chorus, bridge, outro
    float prominence{0.5f};  // 0.0 = background, 1.0 = foreground
    bool isRhythmic{false};
    bool isMelodic{true};
    bool isHarmonic{true};
    std::string tonalCharacter{"neutral"};  // bright, warm, dark, neutral
    
    /**
     * @brief Calculate role compatibility with another instrument
     * @param other Other musical role info
     * @return Compatibility score (0.0-1.0)
     */
    [[nodiscard]] CompatibilityScore calculateCompatibility(const MusicalRoleInfo& other) const noexcept;
};

/**
 * @brief Layering and arrangement information
 */
struct LayeringInfo {
    ArrangementLayer preferredLayer{ArrangementLayer::Midground};
    std::string frequencyRange{"mid"};  // low, low-mid, mid, high-mid, high, full
    float stereoWidth{0.5f};  // 0.0 = mono, 1.0 = wide stereo
    std::string arrangementPosition{"any"};
    float mixPriority{0.5f};  // 0.0 = low priority, 1.0 = high priority
    bool canDoubleOctave{false};
    int maxSimultaneousInstances{1};
    
    /**
     * @brief Calculate layering compatibility
     * @param other Other layering info
     * @return Compatibility score (0.0-1.0)
     */
    [[nodiscard]] CompatibilityScore calculateCompatibility(const LayeringInfo& other) const noexcept;
};

/**
 * @brief Complete audio configuration entry with multi-dimensional metadata
 */
class AudioConfig {
public:
    /**
     * @brief Constructor with required parameters
     * @param id Unique configuration identifier
     * @param name Human-readable name
     * @param configData Original JSON configuration data
     */
    AudioConfig(ConfigId id, std::string name, std::shared_ptr<nlohmann::json> configData);
    
    // Accessors
    [[nodiscard]] const ConfigId& getId() const noexcept { return id_; }
    [[nodiscard]] const std::string& getName() const noexcept { return name_; }
    [[nodiscard]] const std::vector<std::string>& getSemanticTags() const noexcept { return semanticTags_; }
    [[nodiscard]] const EmbeddingVector& getEmbedding() const noexcept { return embedding_; }
    [[nodiscard]] const TechnicalSpecs& getTechnicalSpecs() const noexcept { return techSpecs_; }
    [[nodiscard]] const MusicalRoleInfo& getMusicalRole() const noexcept { return musicalRole_; }
    [[nodiscard]] const LayeringInfo& getLayeringInfo() const noexcept { return layeringInfo_; }
    [[nodiscard]] const nlohmann::json& getConfigData() const;
    
    /**
     * @brief Get normalized tokens for this configuration (v1.4)
     * @return Vector of normalized tokens from ID and tags
     * 
     * Includes tokens from:
     * - Configuration ID (split camelCase/snake_case)
     * - All semantic tags (normalized)
     */
    [[nodiscard]] std::vector<std::string> getAllTokens() const;
    
    // Mutators
    void setSemanticTags(std::vector<std::string> tags);
    void setEmbedding(const EmbeddingVector& embedding);  // Auto-normalizes
    void setTechnicalSpecs(TechnicalSpecs specs) { techSpecs_ = std::move(specs); }
    void setMusicalRole(MusicalRoleInfo role) { musicalRole_ = std::move(role); }
    void setLayeringInfo(LayeringInfo layering) { layeringInfo_ = std::move(layering); }
    
    /**
     * @brief Calculate semantic similarity with another configuration
     * @param other Other audio configuration
     * @return Similarity score (0.0-1.0)
     */
    [[nodiscard]] CompatibilityScore calculateSemanticSimilarity(const AudioConfig& other) const noexcept;

private:
    ConfigId id_;
    std::string name_;
    std::shared_ptr<nlohmann::json> configData_;
    std::vector<std::string> semanticTags_;
    TagSet tagSet_;  // Cached tag set for O(1) intersection
    EmbeddingVector embedding_{};  // Pre-normalized at ingest
    TechnicalSpecs techSpecs_;
    MusicalRoleInfo musicalRole_;
    LayeringInfo layeringInfo_;
    
    // v1.4: Cached normalized tokens for efficient per-token matching
    mutable std::vector<std::string> cachedTokens_;
    mutable bool tokensCached_{false};
};

/**
 * @brief Multi-dimensional compatibility result
 */
struct CompatibilityResult {
    CompatibilityScore overallScore{0.0f};
    CompatibilityScore semanticScore{0.0f};
    CompatibilityScore technicalScore{0.0f};
    CompatibilityScore musicalRoleScore{0.0f};
    CompatibilityScore layeringScore{0.0f};
    
    bool isRecommended{false};
    std::vector<std::string> strengths;
    std::vector<std::string> issues;
    std::vector<std::string> warnings;
    std::unordered_map<std::string, std::string> suggestions;
    
    /**
     * @brief Generate detailed explanation of compatibility
     * @return Human-readable explanation
     */
    [[nodiscard]] std::string generateExplanation() const;
};

/**
 * @brief Scoring weights for multi-dimensional analysis
 */
struct ScoringWeights {
    ScoreWeight semantic{0.2f};
    ScoreWeight technical{0.3f};
    ScoreWeight musicalRole{0.3f};
    ScoreWeight layering{0.2f};
    
    /**
     * @brief Validate that weights sum to approximately 1.0
     * @return True if weights are valid
     */
    [[nodiscard]] bool isValid() const noexcept;
    
    /**
     * @brief Load weights from JSON configuration
     * @param configPath Path to weights configuration file
     * @return Loaded weights or default if loading fails
     */
    static ScoringWeights loadFromConfig(const std::string& configPath);
};

/**
 * @brief User interaction context and preferences
 */
class UserContext {
public:
    // User preferences and learning
    void recordPositiveChoice(const ConfigId& configId) { positiveChoices_.push_back(configId); }
    void recordNegativeChoice(const ConfigId& configId) { negativeChoices_.push_back(configId); }
    void excludeConfig(const ConfigId& configId) { excludedConfigs_.insert(configId); }
    
    // Accessors
    [[nodiscard]] const std::vector<ConfigId>& getSelectedConfigs() const noexcept { return selectedConfigs_; }
    [[nodiscard]] const std::unordered_map<ConfigId, float>& getConfigBoosts() const noexcept { return configBoosts_; }
    [[nodiscard]] bool isExcluded(const ConfigId& configId) const noexcept;
    
    // Context management
    void selectConfig(const ConfigId& configId);
    void deselectConfig(const ConfigId& configId);
    void clearSelection();
    
    /**
     * @brief Calculate user preference boost for a configuration
     * @param configId Configuration identifier
     * @return Boost multiplier (0.1-2.0)
     */
    [[nodiscard]] float calculateUserBoost(const ConfigId& configId) const noexcept;
    
    /**
     * @brief Get search interest tracker (v1.5)
     * @return Reference to tracker
     */
    SearchInterestTracker& getSearchTracker() noexcept { return searchTracker_; }
    const SearchInterestTracker& getSearchTracker() const noexcept { return searchTracker_; }

private:
    std::vector<ConfigId> selectedConfigs_;
    std::vector<ConfigId> positiveChoices_;
    std::vector<ConfigId> negativeChoices_;
    std::unordered_set<ConfigId> excludedConfigs_;
    std::unordered_map<ConfigId, float> configBoosts_;
    std::unordered_map<MusicalRole, float> rolePreferences_;
    
    // v1.5: Search interest tracking with decay
    SearchInterestTracker searchTracker_;
};

/**
 * @brief Embedding engine (v1.6: wraps SemanticKnowledgeBase)
 * 
 * Legacy API preserved for backward compatibility.
 * Internally delegates to SemanticKnowledgeBase for unlimited vocabulary.
 */
class EmbeddingEngine {
public:
    /**
     * @brief Initialize with default knowledge base
     */
    EmbeddingEngine();
    
    /**
     * @brief Load semantic database (v1.6: SQLite instead of JSON)
     * @param dbPath Path to semantic database file (.db)
     * @return True if loaded successfully
     * 
     * v1.6: Loads SQLite database with embeddings, aliases, IDF stats
     * v1.5: JSON format (still supported via migration)
     */
    bool loadEmbeddingIndex(const std::string& dbPath);
    
    /**
     * @brief Get embedding for arbitrary text (v1.6: unlimited vocabulary)
     * @param text Input text (any words, phrases, free-form descriptions)
     * @return Dynamic-dimension embedding vector (unit-normalized)
     * 
     * v1.6: Uses SemanticKnowledgeBase (DB lookup + sentence encoder fallback)
     * Guarantees: Non-empty, unit-length vector even if DB is empty
     */
    [[nodiscard]] EmbeddingVector getEmbedding(const std::string& text) const;
    
    /**
     * @brief Calculate cosine similarity between PRE-NORMALIZED embeddings
     * @param a First embedding (must be normalized)
     * @param b Second embedding (must be normalized)
     * @return Similarity score [0.0, 1.0]
     * 
     * v1.6: Handles dynamic dimensions, asserts matching sizes
     */
    [[nodiscard]] static CompatibilityScore calculateSimilarity(
        const EmbeddingVector& a, 
        const EmbeddingVector& b) noexcept;
    
    /**
     * @brief Calculate weighted cosine similarity with diagonal weights
     * @param a First embedding (normalized)
     * @param b Second embedding (normalized)
     * @param weights Diagonal weight vector (optional)
     * @return Weighted similarity score [0.0, 1.0]
     */
    [[nodiscard]] static CompatibilityScore calculateWeightedSimilarity(
        const EmbeddingVector& a, 
        const EmbeddingVector& b,
        const EmbeddingVector* weights = nullptr) noexcept;
    
    /**
     * @brief Normalize embedding to unit length (in-place)
     * @param embedding Embedding to normalize
     * 
     * v1.6: Works with dynamic-dimension vectors
     */
    static void normalizeEmbedding(EmbeddingVector& embedding) noexcept;
    
    /**
     * @brief Find most similar words to given embedding
     * @param embedding Target embedding
     * @param topK Number of results to return
     * @return Vector of (word, similarity) pairs
     */
    [[nodiscard]] std::vector<std::pair<std::string, float>> findSimilarWords(
        const EmbeddingVector& embedding, int topK = 5) const;
    
    /**
     * @brief Get tag IDF (inverse document frequency) for weighting
     * @param tag Tag name
     * @return IDF weight (higher = more discriminative)
     */
    [[nodiscard]] float getTagIDF(const std::string& tag) const noexcept;
    
    /**
     * @brief Update tag document frequencies (call during initialization)
     * @param allTags Vector of all tag sets from all configurations
     */
    void updateTagStatistics(const std::vector<std::vector<std::string>>& allTags);
    
    /**
     * @brief Get embedding dimension
     * @return Current embedding dimension
     */
    [[nodiscard]] int getDimension() const noexcept;
    
    /**
     * @brief Check if knowledge base is ready
     * @return true if initialized
     */
    [[nodiscard]] bool isReady() const noexcept;
    
    /**
     * @brief Get access to underlying knowledge base (for persistence operations)
     * @return Pointer to knowledge base (nullptr if not initialized)
     */
    SemanticKnowledgeBase* getKnowledgeBase() const noexcept;

private:
    // v1.6: Delegate to SemanticKnowledgeBase
    std::unique_ptr<SemanticKnowledgeBase> knowledgeBase_;
    int dimension_{100};
    bool isReady_{false};
    
    // Legacy members removed (no longer needed):
    // - wordEmbeddings_ (replaced by SemanticDatabase)
    // - subwordEmbeddings_ (replaced by SentenceEncoder)
    // - tagIDF_ (stored in SemanticDatabase)
    // - usingSKDIndex_ (always true in v1.6)
};

/**
 * @brief Multi-dimensional pointer for compatibility analysis
 */
class MultiDimensionalPointer {
public:
    /**
     * @brief Constructor with scoring weights
     * @param weights Scoring weights for each dimension
     * @param embeddingEngine Shared embedding engine
     */
    explicit MultiDimensionalPointer(ScoringWeights weights, std::shared_ptr<EmbeddingEngine> embeddingEngine);
    
    /**
     * @brief Analyze compatibility between two configurations
     * @param configA First configuration
     * @param configB Second configuration
     * @return Detailed compatibility result
     */
    [[nodiscard]] CompatibilityResult analyzeCompatibility(const AudioConfig& configA, const AudioConfig& configB) const;
    
    /**
     * @brief Find compatible configurations for a given anchor
     * @param anchor Anchor configuration
     * @param candidates Candidate configurations
     * @param userContext User preferences and exclusions
     * @param maxResults Maximum number of results
     * @return Sorted list of compatible configurations with scores
     */
    [[nodiscard]] std::vector<std::pair<std::shared_ptr<AudioConfig>, CompatibilityResult>> 
    findCompatibleConfigurations(
        const AudioConfig& anchor,
        const std::vector<std::shared_ptr<AudioConfig>>& candidates,
        const UserContext& userContext,
        int maxResults = 10) const;
    
    /**
     * @brief Update scoring weights
     * @param newWeights New weights to use
     */
    void updateWeights(const ScoringWeights& newWeights) { weights_ = newWeights; }

private:
    ScoringWeights weights_;
    std::shared_ptr<EmbeddingEngine> embeddingEngine_;
    
    [[nodiscard]] CompatibilityScore calculateSemanticScore(const AudioConfig& a, const AudioConfig& b) const;
    [[nodiscard]] CompatibilityScore calculateTechnicalScore(const AudioConfig& a, const AudioConfig& b) const;
    [[nodiscard]] CompatibilityScore calculateMusicalRoleScore(const AudioConfig& a, const AudioConfig& b) const;
    [[nodiscard]] CompatibilityScore calculateLayeringScore(const AudioConfig& a, const AudioConfig& b) const;
};

/**
 * @brief Configuration generator for synthesis-ready output
 */
class ConfigGenerator {
public:
    /**
     * @brief Generate synthesis-ready configuration from selected audio configs
     * @param selectedConfigs Vector of selected configurations
     * @param userContext User preferences and context
     * @return JSON configuration ready for synthesis
     */
    [[nodiscard]] static std::shared_ptr<nlohmann::json> generateSynthesisConfig(
        const std::vector<std::shared_ptr<AudioConfig>>& selectedConfigs,
        const UserContext& userContext);
    
    /**
     * @brief Validate configuration chain for technical compatibility
     * @param configChain Vector of configurations in order
     * @return Validation result with issues and suggestions
     */
    [[nodiscard]] static CompatibilityResult validateConfigChain(
        const std::vector<std::shared_ptr<AudioConfig>>& configChain);

private:
    static void mergeConfigData(nlohmann::json& target, const nlohmann::json& source);
    static void applyUserPreferences(nlohmann::json& config, const UserContext& userContext);
};

/**
 * @brief Main audio configuration system
 */
class AudioConfigSystem {
public:
    /**
     * @brief Constructor with configuration file path
     * @param weightsConfigPath Path to scoring weights configuration
     */
    explicit AudioConfigSystem(const std::string& weightsConfigPath);
    
    /**
     * @brief Initialize the system with configuration database
     * @param configDatabasePath Path to clean configuration JSON
     * @param skdIndexPath Optional path to SKD embedding index (empty = use built-in)
     * @return True if initialization succeeded
     */
    [[nodiscard]] bool initialize(const std::string& configDatabasePath, 
                                   const std::string& skdIndexPath = "");
    
    /**
     * @brief Run interactive command-line interface
     */
    void runInteractiveCLI();
    
    /**
     * @brief Search configurations by semantic query
     * @param query Search query string
     * @param maxResults Maximum number of results
     * @return Vector of matching configurations with scores
     */
    [[nodiscard]] std::vector<std::pair<std::shared_ptr<AudioConfig>, CompatibilityScore>>
    searchConfigurations(const std::string& query, int maxResults = 10) const;
    
    /**
     * @brief Get configuration by ID
     * @param configId Configuration identifier
     * @return Configuration pointer or nullptr if not found
     */
    [[nodiscard]] std::shared_ptr<AudioConfig> getConfiguration(const ConfigId& configId) const;
    
    /**
     * @brief Generate synthesis configuration from current user selection
     * @param outputPath Path to write generated configuration
     * @return True if generation succeeded
     */
    [[nodiscard]] bool generateSynthesisConfiguration(const std::string& outputPath) const;
    
    /**
     * @brief Synchronize knowledge base (refresh embeddings, persist signals)
     * @return True if sync succeeded
     * 
     * This method:
     * - Re-encodes tags and updates embeddings if encoder is available
     * - Recomputes IDF statistics from current configuration corpus
     * - Persists user search interest signals to database
     */
    [[nodiscard]] bool syncKnowledgeBase();

private:
    std::shared_ptr<EmbeddingEngine> embeddingEngine_;
    std::unique_ptr<MultiDimensionalPointer> pointer_;
    std::unordered_map<ConfigId, std::shared_ptr<AudioConfig>> configurations_;
    UserContext userContext_;
    ScoringWeights weights_;
    
    // CLI command handlers
    void handleSearchCommand(const std::vector<std::string>& args);
    void handleSelectCommand(const std::vector<std::string>& args);
    void handleBoostCommand(const std::vector<std::string>& args);
    void handleDemoteCommand(const std::vector<std::string>& args);
    void handleExcludeCommand(const std::vector<std::string>& args);
    void handleListCommand(const std::vector<std::string>& args);
    void handleStatsCommand(const std::vector<std::string>& args);
    void handleKBStatsCommand(const std::vector<std::string>& args);  // v1.6
    void handleGenerateCommand(const std::vector<std::string>& args);
    void handleHelpCommand(const std::vector<std::string>& args);
    void handleExamplesCommand(const std::vector<std::string>& args);
    void handleSignalsCommand(const std::vector<std::string>& args);  // v1.5
    
    // Helper methods
    void loadConfigurationDatabase(const std::string& configPath);
    void printConfigurationSummary(const AudioConfig& config, CompatibilityScore score = -1.0f) const;
    void printCompatibilityResult(const CompatibilityResult& result) const;
    [[nodiscard]] std::vector<std::string> tokenizeCommand(const std::string& command) const;
};

} // namespace audio_config
