#pragma once

#include "common_types.h"
#include "audio_config.h"
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <memory>

/**
 * @file ai_scorer.h
 * @brief AI-driven semantic scoring and configuration matching system
 * @author AI Synthesis Team
 * @version 1.0
 */

namespace AISynthesis {

/**
 * @brief Semantic keyword database entry
 * 
 * Contains information about a semantic keyword including its category,
 * aliases, and scoring weight for AI matching algorithms.
 */
struct SemanticKeywordEntry {
    std::string category{};                    ///< Category (timbral, emotional, dynamic, material)
    std::vector<std::string> aliases{};        ///< Alternative names for this keyword
    float scoreWeight{1.0f};                   ///< Base scoring weight
    std::vector<double> vectorEmbedding{};     ///< Future: vector embedding for similarity
    
    /**
     * @brief Check if this entry matches a given tag
     * @param tag Tag to match against (case-insensitive)
     * @return true if tag matches this entry or any alias
     */
    bool matchesTag(const std::string& tag) const noexcept;
    
    /**
     * @brief Get similarity score with another tag
     * @param tag Tag to compare against
     * @return Similarity score (0.0 to 1.0)
     */
    float getSimilarityScore(const std::string& tag) const noexcept;
};

/**
 * @brief Semantic Keyword Database (SKD) for AI matching
 * 
 * Manages the database of semantic keywords used by AI for matching
 * user intent to instrument configurations. Provides extensible
 * keyword management and scoring capabilities.
 */
class SemanticKeywordDatabase {
public:
    /**
     * @brief Construct empty semantic keyword database
     */
    explicit SemanticKeywordDatabase() = default;
    
    /**
     * @brief Load keyword database from JSON file
     * @param filePath Path to JSON file containing keyword data
     * @return true if loaded successfully
     */
    bool loadFromFile(const std::string& filePath) noexcept;
    
    /**
     * @brief Load keyword database from JSON data
     * @param jsonData JSON object containing keyword data
     * @return true if loaded successfully
     */
    bool loadFromJson(const nlohmann::json& jsonData) noexcept;
    
    /**
     * @brief Add or update a keyword entry
     * @param keyword Keyword string
     * @param entry Keyword entry data
     */
    void addKeywordEntry(const std::string& keyword, SemanticKeywordEntry entry);
    
    /**
     * @brief Get keyword entry by name
     * @param keyword Keyword to find
     * @return Optional keyword entry if found
     */
    std::optional<SemanticKeywordEntry> getKeywordEntry(const std::string& keyword) const noexcept;
    
    /**
     * @brief Find keyword entries by category
     * @param category Category to search for
     * @return Vector of matching keyword entries
     */
    std::vector<std::pair<std::string, SemanticKeywordEntry>> getKeywordsByCategory(
        const std::string& category) const noexcept;
    
    /**
     * @brief Group user tags by category
     * @param userTags Tags provided by user
     * @return Map of category to matching keywords
     */
    std::map<std::string, std::vector<std::string>> groupTagsByCategory(
        const std::vector<std::string>& userTags) const noexcept;
    
    /**
     * @brief Find best matching keywords for a tag
     * @param tag Tag to match
     * @param maxResults Maximum number of results
     * @return Vector of (keyword, score) pairs sorted by score
     */
    std::vector<std::pair<std::string, float>> findBestMatches(
        const std::string& tag, std::size_t maxResults = 5) const noexcept;
    
    /**
     * @brief Validate database consistency
     * @return true if database is valid and consistent
     */
    bool validateDatabase() const noexcept;
    
    /**
     * @brief Get all available categories
     * @return Vector of category names
     */
    std::vector<std::string> getAllCategories() const noexcept;
    
    /**
     * @brief Get all keywords in database
     * @return Vector of keyword names
     */
    std::vector<std::string> getAllKeywords() const noexcept;

private:
    std::map<std::string, SemanticKeywordEntry> keywordDatabase_;
    
    /**
     * @brief Normalize keyword string for consistent matching
     * @param keyword Keyword to normalize
     * @return Normalized keyword string
     */
    std::string normalizeKeyword(const std::string& keyword) const noexcept;
    
    /**
     * @brief Initialize default keyword database
     */
    void initializeDefaultDatabase();
};

/**
 * @brief AI configuration scorer for semantic matching
 * 
 * Implements AI-driven scoring algorithms to match user intent
 * (expressed as semantic tags) with available instrument configurations.
 * Supports multiple scoring strategies and extensible matching algorithms.
 */
class AiConfigurationScorer {
public:
    /**
     * @brief Scoring strategy for configuration matching
     */
    enum class ScoringStrategy {
        SEMANTIC_ONLY,      ///< Only use semantic keyword matching
        WEIGHTED_HYBRID,    ///< Combine semantic + parameter similarity
        CONTEXTUAL,         ///< Include musical context (section, mood)
        VECTORIZED          ///< Use vector embeddings (future)
    };
    
    /**
     * @brief Scoring result for a configuration
     */
    struct ScoringResult {
        ConfigurationId configurationId{""};   ///< Configuration identifier
        float totalScore{0.0f};                ///< Total computed score
        float semanticScore{0.0f};             ///< Semantic matching score
        float parameterScore{0.0f};            ///< Parameter similarity score
        float contextScore{0.0f};              ///< Contextual bonus score
        std::vector<std::string> matchedTags{};///< Tags that matched
        std::string scoringDetails{};          ///< Detailed scoring explanation
    };
    
    /**
     * @brief Construct AI configuration scorer
     * @param keywordDatabase Reference to semantic keyword database
     * @param strategy Scoring strategy to use
     */
    explicit AiConfigurationScorer(
        const SemanticKeywordDatabase& keywordDatabase,
        ScoringStrategy strategy = ScoringStrategy::WEIGHTED_HYBRID
    );
    
    /**
     * @brief Score a single configuration against user tags
     * @param config Configuration to score
     * @param userTags Tags representing user intent
     * @param musicalContext Optional musical context (section, mood, etc.)
     * @return Scoring result with detailed breakdown
     */
    ScoringResult scoreConfiguration(
        const InstrumentConfig& config,
        const std::vector<std::string>& userTags,
        const std::map<std::string, std::string>& musicalContext = {}
    ) const noexcept;
    
    /**
     * @brief Score multiple configurations and return sorted results
     * @param configurations Vector of configurations to score
     * @param userTags Tags representing user intent
     * @param musicalContext Optional musical context
     * @param maxResults Maximum number of results to return
     * @return Vector of scoring results sorted by total score (descending)
     */
    std::vector<ScoringResult> scoreConfigurations(
        const std::vector<std::reference_wrapper<const InstrumentConfig>>& configurations,
        const std::vector<std::string>& userTags,
        const std::map<std::string, std::string>& musicalContext = {},
        std::size_t maxResults = 10
    ) const noexcept;
    
    /**
     * @brief Filter configurations by minimum score threshold
     * @param configurations Vector of configurations to filter
     * @param userTags Tags representing user intent
     * @param minimumScore Minimum score threshold (0.0 to 1.0)
     * @param musicalContext Optional musical context
     * @return Vector of configurations that meet the threshold
     */
    std::vector<std::reference_wrapper<const InstrumentConfig>> filterByThreshold(
        const std::vector<std::reference_wrapper<const InstrumentConfig>>& configurations,
        const std::vector<std::string>& userTags,
        float minimumScore,
        const std::map<std::string, std::string>& musicalContext = {}
    ) const noexcept;
    
    /**
     * @brief Set scoring strategy
     * @param strategy New scoring strategy
     */
    void setScoringStrategy(ScoringStrategy strategy) noexcept { scoringStrategy_ = strategy; }
    
    /**
     * @brief Get current scoring strategy
     * @return Current scoring strategy
     */
    ScoringStrategy getScoringStrategy() const noexcept { return scoringStrategy_; }
    
    /**
     * @brief Set scoring weights for hybrid strategy
     * @param semanticWeight Weight for semantic matching (0.0 to 1.0)
     * @param parameterWeight Weight for parameter similarity (0.0 to 1.0)
     * @param contextWeight Weight for contextual bonus (0.0 to 1.0)
     */
    void setScoringWeights(float semanticWeight, float parameterWeight, float contextWeight);
    
    /**
     * @brief Get detailed explanation of scoring for a configuration
     * @param config Configuration to explain
     * @param userTags User tags used for matching
     * @param musicalContext Musical context used
     * @return Human-readable explanation of scoring
     */
    std::string explainScoring(
        const InstrumentConfig& config,
        const std::vector<std::string>& userTags,
        const std::map<std::string, std::string>& musicalContext = {}
    ) const noexcept;

private:
    const SemanticKeywordDatabase& keywordDatabase_;
    ScoringStrategy scoringStrategy_;
    
    // Scoring weights for hybrid strategy
    float semanticWeight_{0.6f};
    float parameterWeight_{0.3f};
    float contextWeight_{0.1f};
    
    /**
     * @brief Compute semantic matching score
     * @param config Configuration to score
     * @param userTags User tags
     * @return Semantic score (0.0 to 1.0)
     */
    float computeSemanticScore(
        const InstrumentConfig& config,
        const std::vector<std::string>& userTags
    ) const noexcept;
    
    /**
     * @brief Compute parameter similarity score
     * @param config Configuration to score
     * @param userTags User tags (for parameter inference)
     * @return Parameter score (0.0 to 1.0)
     */
    float computeParameterScore(
        const InstrumentConfig& config,
        const std::vector<std::string>& userTags
    ) const noexcept;
    
    /**
     * @brief Compute contextual bonus score
     * @param config Configuration to score
     * @param musicalContext Musical context map
     * @return Context score (0.0 to 1.0)
     */
    float computeContextScore(
        const InstrumentConfig& config,
        const std::map<std::string, std::string>& musicalContext
    ) const noexcept;
    
    /**
     * @brief Extract searchable tags from configuration
     * @param config Configuration to extract from
     * @return Vector of searchable tag strings
     */
    std::vector<std::string> extractConfigurationTags(const InstrumentConfig& config) const noexcept;
    
    /**
     * @brief Compute cosine similarity between two tag vectors
     * @param tags1 First tag vector
     * @param tags2 Second tag vector
     * @return Similarity score (0.0 to 1.0)
     */
    double computeCosineSimilarity(
        const std::vector<std::string>& tags1,
        const std::vector<std::string>& tags2
    ) const noexcept;
    
    /**
     * @brief Apply quality bonus to score
     * @param baseScore Base score before quality adjustment
     * @param quality Configuration quality level
     * @return Adjusted score with quality bonus
     */
    float applyQualityBonus(float baseScore, ConfigurationQuality quality) const noexcept;
    
    /**
     * @brief Validate scoring weights
     * @return true if weights are valid and normalized
     */
    bool validateScoringWeights() const noexcept;
};

/**
 * @brief Configuration suggestion engine
 * 
 * High-level interface for AI-driven configuration suggestions.
 * Combines semantic scoring with filtering and ranking to provide
 * the best configuration suggestions for user intent.
 */
class ConfigurationSuggestionEngine {
public:
    /**
     * @brief Suggestion request parameters
     */
    struct SuggestionRequest {
        std::vector<std::string> userTags{};                    ///< User-provided semantic tags
        std::map<std::string, std::string> musicalContext{};    ///< Musical context (section, mood, etc.)
        std::size_t maxSuggestions{5};                          ///< Maximum number of suggestions
        float minimumScore{0.1f};                               ///< Minimum score threshold
        bool includeOnlyValid{true};                            ///< Only include valid configurations
        AiConfigurationScorer::ScoringStrategy strategy{
            AiConfigurationScorer::ScoringStrategy::WEIGHTED_HYBRID
        };                                                       ///< Scoring strategy to use
    };
    
    /**
     * @brief Configuration suggestion result
     */
    struct SuggestionResult {
        std::vector<AiConfigurationScorer::ScoringResult> suggestions{};  ///< Ordered suggestions
        std::string recommendationReason{};                               ///< Why these were recommended
        std::map<std::string, std::vector<std::string>> categoryBreakdown{};  ///< Tags grouped by category
        bool hasHighConfidenceMatch{false};                               ///< Whether top match is high confidence
    };
    
    /**
     * @brief Construct suggestion engine
     * @param keywordDatabase Reference to semantic keyword database
     */
    explicit ConfigurationSuggestionEngine(const SemanticKeywordDatabase& keywordDatabase);
    
    /**
     * @brief Get configuration suggestions for user request
     * @param request Suggestion request parameters
     * @param availableConfigurations Available configurations to choose from
     * @return Suggestion result with ranked configurations
     */
    SuggestionResult getSuggestions(
        const SuggestionRequest& request,
        const std::vector<std::reference_wrapper<const InstrumentConfig>>& availableConfigurations
    ) const noexcept;
    
    /**
     * @brief Get suggestions with automatic context inference
     * @param userTags User-provided tags
     * @param availableConfigurations Available configurations
     * @param maxSuggestions Maximum suggestions to return
     * @return Suggestion result with inferred context
     */
    SuggestionResult getSmartSuggestions(
        const std::vector<std::string>& userTags,
        const std::vector<std::reference_wrapper<const InstrumentConfig>>& availableConfigurations,
        std::size_t maxSuggestions = 5
    ) const noexcept;
    
    /**
     * @brief Analyze user tags and provide insights
     * @param userTags Tags to analyze
     * @return Analysis result with category breakdown and suggestions
     */
    std::map<std::string, std::vector<std::string>> analyzeUserTags(
        const std::vector<std::string>& userTags
    ) const noexcept;

private:
    const SemanticKeywordDatabase& keywordDatabase_;
    std::unique_ptr<AiConfigurationScorer> scorer_;
    
    /**
     * @brief Infer musical context from user tags
     * @param userTags User-provided tags
     * @return Inferred context map
     */
    std::map<std::string, std::string> inferMusicalContext(
        const std::vector<std::string>& userTags
    ) const noexcept;
    
    /**
     * @brief Generate recommendation explanation
     * @param suggestions Suggestion results
     * @param request Original request
     * @return Human-readable explanation
     */
    std::string generateRecommendationReason(
        const std::vector<AiConfigurationScorer::ScoringResult>& suggestions,
        const SuggestionRequest& request
    ) const noexcept;
    
    /**
     * @brief Determine if top suggestion has high confidence
     * @param suggestions Suggestion results
     * @return true if top suggestion is high confidence
     */
    bool hasHighConfidenceMatch(
        const std::vector<AiConfigurationScorer::ScoringResult>& suggestions
    ) const noexcept;
};

} // namespace AISynthesis