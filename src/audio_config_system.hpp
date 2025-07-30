/**
 * @file audio_config_system.hpp
 * @brief Multi-Dimensional Audio Configuration System - Main Header
 * @author AI Assistant
 * @version 1.0
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <functional>
#include <array>
#include "json.hpp"

namespace audio_config {

// Type aliases for clarity
using ConfigId = std::string;
using EmbeddingVector = std::array<float, 100>;  // 100D FastText embeddings
using ScoreWeight = float;
using CompatibilityScore = float;

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
    
    // Mutators
    void setSemanticTags(std::vector<std::string> tags) { semanticTags_ = std::move(tags); }
    void setEmbedding(const EmbeddingVector& embedding) { embedding_ = embedding; }
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
    EmbeddingVector embedding_{};
    TechnicalSpecs techSpecs_;
    MusicalRoleInfo musicalRole_;
    LayeringInfo layeringInfo_;
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

private:
    std::vector<ConfigId> selectedConfigs_;
    std::vector<ConfigId> positiveChoices_;
    std::vector<ConfigId> negativeChoices_;
    std::unordered_set<ConfigId> excludedConfigs_;
    std::unordered_map<ConfigId, float> configBoosts_;
    std::unordered_map<MusicalRole, float> rolePreferences_;
};

/**
 * @brief Fast embedding engine with FastText-style capabilities
 */
class EmbeddingEngine {
public:
    /**
     * @brief Initialize with pre-trained embeddings
     */
    EmbeddingEngine();
    
    /**
     * @brief Get embedding for a word or phrase
     * @param text Input text
     * @return 100D embedding vector
     */
    [[nodiscard]] EmbeddingVector getEmbedding(const std::string& text) const;
    
    /**
     * @brief Calculate cosine similarity between embeddings
     * @param a First embedding
     * @param b Second embedding
     * @return Similarity score (0.0-1.0)
     */
    [[nodiscard]] static CompatibilityScore calculateSimilarity(const EmbeddingVector& a, const EmbeddingVector& b) noexcept;
    
    /**
     * @brief Find most similar words to given embedding
     * @param embedding Target embedding
     * @param topK Number of results to return
     * @return Vector of (word, similarity) pairs
     */
    [[nodiscard]] std::vector<std::pair<std::string, float>> findSimilarWords(
        const EmbeddingVector& embedding, int topK = 5) const;

private:
    std::unordered_map<std::string, EmbeddingVector> wordEmbeddings_;
    std::unordered_map<std::string, EmbeddingVector> subwordEmbeddings_;
    
    void loadPretrainedEmbeddings();
    void generateSubwordEmbeddings();
    [[nodiscard]] EmbeddingVector computeTextEmbedding(const std::string& text) const;
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
     * @return True if initialization succeeded
     */
    [[nodiscard]] bool initialize(const std::string& configDatabasePath);
    
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
    void handleGenerateCommand(const std::vector<std::string>& args);
    void handleHelpCommand(const std::vector<std::string>& args);
    void handleExamplesCommand(const std::vector<std::string>& args);
    
    // Helper methods
    void loadConfigurationDatabase(const std::string& configPath);
    void printConfigurationSummary(const AudioConfig& config, CompatibilityScore score = -1.0f) const;
    void printCompatibilityResult(const CompatibilityResult& result) const;
    [[nodiscard]] std::vector<std::string> tokenizeCommand(const std::string& command) const;
};

} // namespace audio_config