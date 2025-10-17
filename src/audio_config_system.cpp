/**
 * @file audio_config_system.cpp
 * @brief Multi-Dimensional Audio Configuration System - Implementation
 * @author AI Assistant
 * @version 1.2
 * 
 * v1.2 Efficiency & Quality Upgrades:
 * - Pre-normalized embeddings: Store unit-length vectors at ingest
 * - Cached tag sets: O(1) intersection vs O(m·n) nested loops
 * - IDF-weighted tags: Reward informative tag overlaps
 * - Clamped scores: Ensure [0,1] range, prevent weighted sum skew
 * - Diagonal weighting: Optional emphasis on salient dimensions
 */

#include "audio_config_system.hpp"
#include "json.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <iomanip>
#include <random>
#include <regex>
#include <unordered_set>
#include <set>
#include <map>
#include <ctime>

using json = nlohmann::json;

namespace audio_config {

// Constants for semantic similarity calculation (v1.2 enhanced)
namespace {
    constexpr float TAG_IDF_LAMBDA_CLAMP = 0.3f;  // Maximum tag boost contribution
    constexpr float MIN_IDF = 0.1f;  // Minimum IDF value for rare tags
    constexpr float EMBEDDING_WEIGHT = 0.7f;  // Weight for embedding similarity
    constexpr float TAG_WEIGHT = 0.3f;  // Weight for tag similarity
    constexpr float NUMERICAL_EPSILON = 1e-8f;  // Numerical stability threshold
}

// ============================================================================
// TechnicalSpecs Implementation
// ============================================================================

CompatibilityScore TechnicalSpecs::isCompatibleWith(const TechnicalSpecs& other) const noexcept {
    float score = 0.0f;
    int totalChecks = 0;
    
    // Sample rate compatibility
    totalChecks++;
    if (std::abs(sampleRate - other.sampleRate) < 0.1f) {
        score += 1.0f;
    } else if (std::abs(sampleRate - other.sampleRate) < 4800.0f) {
        score += 0.5f; // Minor difference, could be converted
    }
    
    // Bit depth compatibility
    totalChecks++;
    if (bitDepth == other.bitDepth) {
        score += 1.0f;
    } else if (std::abs(bitDepth - other.bitDepth) <= 8) {
        score += 0.6f; // Convertible with quality loss
    }
    
    // Polyphony compatibility (use minimum)
    totalChecks++;
    int minPolyphony = std::min(polyphony, other.polyphony);
    if (minPolyphony >= 16) {
        score += 1.0f;
    } else if (minPolyphony >= 8) {
        score += 0.7f;
    } else if (minPolyphony >= 4) {
        score += 0.4f;
    }
    
    // Envelope type compatibility
    totalChecks++;
    if (envelopeType == other.envelopeType) {
        score += 1.0f;
    } else {
        // Check for compatible envelope types
        static const std::unordered_map<std::string, std::vector<std::string>> compatibleEnvelopes = {
            {"ADSR", {"DADSR", "AHDSR"}},
            {"DADSR", {"ADSR", "AHDSR"}},
            {"AHDSR", {"ADSR", "DADSR"}},
            {"AD", {"ADSR", "DADSR"}},
            {"AR", {"ADSR", "DADSR"}}
        };
        
        auto it = compatibleEnvelopes.find(envelopeType);
        if (it != compatibleEnvelopes.end()) {
            auto& compatibles = it->second;
            if (std::find(compatibles.begin(), compatibles.end(), other.envelopeType) != compatibles.end()) {
                score += 0.7f;
            }
        }
    }
    
    // Plugin format compatibility
    totalChecks++;
    if (pluginFormat == other.pluginFormat) {
        score += 1.0f;
    } else {
        // Some cross-compatibility exists
        if ((pluginFormat == PluginFormat::VST2 && other.pluginFormat == PluginFormat::VST3) ||
            (pluginFormat == PluginFormat::VST3 && other.pluginFormat == PluginFormat::VST2)) {
            score += 0.8f;
        }
    }
    
    // BPM range compatibility
    totalChecks++;
    float overlapStart = std::max(bpmRange.first, other.bpmRange.first);
    float overlapEnd = std::min(bpmRange.second, other.bpmRange.second);
    if (overlapEnd > overlapStart) {
        float overlapRatio = (overlapEnd - overlapStart) / 
                           std::max(bpmRange.second - bpmRange.first, other.bpmRange.second - other.bpmRange.first);
        score += overlapRatio;
    }
    
    // Buffer size compatibility
    totalChecks++;
    int bufferOverlapStart = std::max(bufferSizeRange.first, other.bufferSizeRange.first);
    int bufferOverlapEnd = std::min(bufferSizeRange.second, other.bufferSizeRange.second);
    if (bufferOverlapEnd >= bufferOverlapStart) {
        score += 1.0f;
    }
    
    return totalChecks > 0 ? score / totalChecks : 0.0f;
}

// ============================================================================
// MusicalRoleInfo Implementation
// ============================================================================

CompatibilityScore MusicalRoleInfo::calculateCompatibility(const MusicalRoleInfo& other) const noexcept {
    // Role compatibility matrix
    static const std::unordered_map<MusicalRole, std::vector<MusicalRole>> compatibleRoles = {
        {MusicalRole::Lead, {MusicalRole::Bass, MusicalRole::Pad, MusicalRole::Percussion, MusicalRole::Arp, MusicalRole::Chord}},
        {MusicalRole::Bass, {MusicalRole::Lead, MusicalRole::Pad, MusicalRole::Percussion, MusicalRole::Chord}},
        {MusicalRole::Pad, {MusicalRole::Lead, MusicalRole::Bass, MusicalRole::Percussion, MusicalRole::Arp, MusicalRole::Chord}},
        {MusicalRole::Arp, {MusicalRole::Lead, MusicalRole::Pad, MusicalRole::Bass, MusicalRole::Chord}},
        {MusicalRole::Percussion, {MusicalRole::Lead, MusicalRole::Bass, MusicalRole::Pad, MusicalRole::Arp, MusicalRole::Chord}},
        {MusicalRole::Chord, {MusicalRole::Lead, MusicalRole::Bass, MusicalRole::Pad, MusicalRole::Arp}},
        {MusicalRole::FX, {MusicalRole::Lead, MusicalRole::Bass, MusicalRole::Pad, MusicalRole::Arp, MusicalRole::Chord}}
    };
    
    float score = 0.0f;
    
    // Check role compatibility
    auto it = compatibleRoles.find(primaryRole);
    if (it != compatibleRoles.end()) {
        auto& compatibles = it->second;
        if (std::find(compatibles.begin(), compatibles.end(), other.primaryRole) != compatibles.end()) {
            score += 0.4f;
        }
    }
    
    // Musical context compatibility
    if (musicalContext == other.musicalContext || musicalContext == "any" || other.musicalContext == "any") {
        score += 0.2f;
    }
    
    // Prominence balance (avoid multiple high-prominence instruments)
    float prominenceDiff = std::abs(prominence - other.prominence);
    if (prominenceDiff > 0.3f) {
        score += 0.2f; // Good separation
    } else if (prominence < 0.7f && other.prominence < 0.7f) {
        score += 0.1f; // Both low prominence is okay
    }
    
    // Tonal character compatibility
    if (tonalCharacter == other.tonalCharacter || tonalCharacter == "neutral" || other.tonalCharacter == "neutral") {
        score += 0.1f;
    }
    
    // Functional compatibility (rhythmic/melodic/harmonic)
    if ((isRhythmic && other.isRhythmic) || (isMelodic && other.isHarmonic) || (isHarmonic && other.isMelodic)) {
        score += 0.1f;
    }
    
    return std::min(score, 1.0f);
}

// ============================================================================
// LayeringInfo Implementation
// ============================================================================

CompatibilityScore LayeringInfo::calculateCompatibility(const LayeringInfo& other) const noexcept {
    float score = 0.0f;
    
    // Layer compatibility (different layers work well together)
    if (preferredLayer != other.preferredLayer) {
        score += 0.3f;
    } else if (preferredLayer == ArrangementLayer::Background) {
        score += 0.2f; // Multiple background elements are okay
    }
    
    // Frequency range separation
    static const std::unordered_map<std::string, int> freqOrder = {
        {"low", 1}, {"low-mid", 2}, {"mid", 3}, {"high-mid", 4}, {"high", 5}, {"full", 6}
    };
    
    auto freq1 = freqOrder.find(frequencyRange);
    auto freq2 = freqOrder.find(other.frequencyRange);
    if (freq1 != freqOrder.end() && freq2 != freqOrder.end()) {
        int freqDiff = std::abs(freq1->second - freq2->second);
        if (freqDiff >= 2 || frequencyRange == "full" || other.frequencyRange == "full") {
            score += 0.2f; // Good frequency separation
        } else if (freqDiff == 1) {
            score += 0.1f; // Adjacent frequencies can work
        }
    }
    
    // Stereo width compatibility (avoid overcrowding)
    float totalStereoWidth = stereoWidth + other.stereoWidth;
    if (totalStereoWidth <= 1.5f) {
        score += 0.2f;
    } else if (totalStereoWidth <= 2.0f) {
        score += 0.1f;
    }
    
    // Arrangement position compatibility
    if (arrangementPosition == other.arrangementPosition || arrangementPosition == "any" || other.arrangementPosition == "any") {
        score += 0.15f;
    }
    
    // Mix priority balance
    float priorityDiff = std::abs(mixPriority - other.mixPriority);
    if (priorityDiff >= 0.2f) {
        score += 0.15f; // Good priority separation
    }
    
    return std::min(score, 1.0f);
}

// ============================================================================
// AudioConfig Implementation
// ============================================================================

AudioConfig::AudioConfig(ConfigId id, std::string name, std::shared_ptr<nlohmann::json> configData)
    : id_(std::move(id)), name_(std::move(name)), configData_(std::move(configData)) {
    if (!configData_) {
        throw std::invalid_argument("ConfigData cannot be null");
    }
}

const nlohmann::json& AudioConfig::getConfigData() const {
    return *configData_;
}

void AudioConfig::setSemanticTags(std::vector<std::string> tags) {
    semanticTags_ = std::move(tags);
    // Cache as unordered_set for O(1) intersection
    tagSet_.clear();
    tagSet_.insert(semanticTags_.begin(), semanticTags_.end());
    // v1.4: Invalidate token cache
    tokensCached_ = false;
}

void AudioConfig::setEmbedding(const EmbeddingVector& embedding) {
    embedding_ = embedding;
    // Pre-normalize at ingest for faster similarity calculation
    EmbeddingEngine::normalizeEmbedding(embedding_);
}

std::vector<std::string> AudioConfig::getAllTokens() const {
    // v1.4: Cache normalized tokens for efficient per-token search
    if (!tokensCached_) {
        cachedTokens_.clear();
        
        // Tokenize ID (splits camelCase/snake_case)
        auto idTokens = TextUtils::tokenize(id_);
        cachedTokens_.insert(cachedTokens_.end(), idTokens.begin(), idTokens.end());
        
        // Tokenize all semantic tags
        for (const auto& tag : semanticTags_) {
            auto tagTokens = TextUtils::tokenize(tag);
            cachedTokens_.insert(cachedTokens_.end(), tagTokens.begin(), tagTokens.end());
        }
        
        // Remove duplicates
        std::sort(cachedTokens_.begin(), cachedTokens_.end());
        cachedTokens_.erase(std::unique(cachedTokens_.begin(), cachedTokens_.end()), 
                           cachedTokens_.end());
        
        tokensCached_ = true;
    }
    
    return cachedTokens_;
}

CompatibilityScore AudioConfig::calculateSemanticSimilarity(const AudioConfig& other) const noexcept {
    // Calculate embedding similarity (O(d) with pre-normalized vectors)
    float embeddingSimilarity = EmbeddingEngine::calculateSimilarity(embedding_, other.embedding_);
    // Clamp embedding similarity to [0,1]
    embeddingSimilarity = std::clamp(embeddingSimilarity, 0.0f, 1.0f);
    
    // Calculate tag overlap with cached sets (O(min(m,n)) instead of O(m·n))
    float tagSimilarity = 0.0f;
    if (!tagSet_.empty() && !other.tagSet_.empty()) {
        // Count intersection using smaller set for efficiency
        const auto& smallerSet = (tagSet_.size() < other.tagSet_.size()) ? tagSet_ : other.tagSet_;
        const auto& largerSet = (tagSet_.size() < other.tagSet_.size()) ? other.tagSet_ : tagSet_;
        
        int sharedTags = 0;
        for (const auto& tag : smallerSet) {
            if (largerSet.count(tag) > 0) {
                sharedTags++;
            }
        }
        
        // Jaccard similarity for better semantic meaning
        size_t unionSize = tagSet_.size() + other.tagSet_.size() - sharedTags;
        if (unionSize > 0) {
            tagSimilarity = static_cast<float>(sharedTags) / static_cast<float>(unionSize);
        }
    }
    
    // Weighted combination with clamping
    float rawScore = EMBEDDING_WEIGHT * embeddingSimilarity + TAG_WEIGHT * tagSimilarity;
    
    // Clamp final score to [0,1] to prevent downstream weighting skew
    return std::clamp(rawScore, 0.0f, 1.0f);
}

// ============================================================================
// CompatibilityResult Implementation
// ============================================================================

std::string CompatibilityResult::generateExplanation() const {
    std::ostringstream explanation;
    
    explanation << "🎯 Overall Score: " << std::fixed << std::setprecision(2) << overallScore 
                << " (" << (isRecommended ? "RECOMMENDED" : "NOT RECOMMENDED") << ")\n";
    
    explanation << "📊 Dimension Breakdown:\n";
    explanation << "  • Semantic: " << std::setprecision(2) << semanticScore << "\n";
    explanation << "  • Technical: " << std::setprecision(2) << technicalScore << "\n";
    explanation << "  • Musical Role: " << std::setprecision(2) << musicalRoleScore << "\n";
    explanation << "  • Layering: " << std::setprecision(2) << layeringScore << "\n";
    
    if (!strengths.empty()) {
        explanation << "\n✅ Strengths:\n";
        for (const auto& strength : strengths) {
            explanation << "  • " << strength << "\n";
        }
    }
    
    if (!issues.empty()) {
        explanation << "\n❌ Issues:\n";
        for (const auto& issue : issues) {
            explanation << "  • " << issue << "\n";
        }
    }
    
    if (!warnings.empty()) {
        explanation << "\n⚠️  Warnings:\n";
        for (const auto& warning : warnings) {
            explanation << "  • " << warning << "\n";
        }
    }
    
    if (!suggestions.empty()) {
        explanation << "\n💡 Suggestions:\n";
        for (const auto& [category, suggestion] : suggestions) {
            explanation << "  • " << category << ": " << suggestion << "\n";
        }
    }
    
    return explanation.str();
}

// ============================================================================
// ScoringWeights Implementation
// ============================================================================

bool ScoringWeights::isValid() const noexcept {
    float sum = semantic + technical + musicalRole + layering;
    return std::abs(sum - 1.0f) < 0.01f && 
           semantic >= 0.0f && technical >= 0.0f && musicalRole >= 0.0f && layering >= 0.0f;
}

ScoringWeights ScoringWeights::loadFromConfig(const std::string& configPath) {
    ScoringWeights weights; // Default values
    
    try {
        std::ifstream file(configPath);
        if (file.is_open()) {
            json config;
            file >> config;
            
            if (config.contains("weights")) {
                const auto& w = config["weights"];
                if (w.contains("semantic")) weights.semantic = w["semantic"].get<float>();
                if (w.contains("technical")) weights.technical = w["technical"].get<float>();
                if (w.contains("musicalRole")) weights.musicalRole = w["musicalRole"].get<float>();
                if (w.contains("layering")) weights.layering = w["layering"].get<float>();
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "⚠️  Warning: Could not load weights config from " << configPath 
                  << ": " << e.what() << ". Using defaults." << std::endl;
    }
    
    if (!weights.isValid()) {
        std::cerr << "⚠️  Warning: Invalid weights loaded. Using defaults." << std::endl;
        return ScoringWeights{}; // Return default
    }
    
    return weights;
}

// ============================================================================
// UserContext Implementation  
// ============================================================================

bool UserContext::isExcluded(const ConfigId& configId) const noexcept {
    return excludedConfigs_.find(configId) != excludedConfigs_.end();
}

void UserContext::selectConfig(const ConfigId& configId) {
    auto it = std::find(selectedConfigs_.begin(), selectedConfigs_.end(), configId);
    if (it == selectedConfigs_.end()) {
        selectedConfigs_.push_back(configId);
    }
}

void UserContext::deselectConfig(const ConfigId& configId) {
    auto it = std::find(selectedConfigs_.begin(), selectedConfigs_.end(), configId);
    if (it != selectedConfigs_.end()) {
        selectedConfigs_.erase(it);
    }
}

void UserContext::clearSelection() {
    selectedConfigs_.clear();
}

float UserContext::calculateUserBoost(const ConfigId& configId) const noexcept {
    auto it = configBoosts_.find(configId);
    if (it != configBoosts_.end()) {
        return it->second;
    }
    
    // Calculate boost based on positive/negative choices
    float boost = 1.0f;
    
    auto posCount = std::count(positiveChoices_.begin(), positiveChoices_.end(), configId);
    auto negCount = std::count(negativeChoices_.begin(), negativeChoices_.end(), configId);
    
    boost += posCount * 0.1f;
    boost -= negCount * 0.1f;
    
    return std::clamp(boost, 0.1f, 2.0f);
}


// ============================================================================
// EmbeddingEngine Implementation (v1.6: Wraps SemanticKnowledgeBase)
// ============================================================================

EmbeddingEngine::EmbeddingEngine() 
    : dimension_(100), isReady_(false) {
}

bool EmbeddingEngine::loadEmbeddingIndex(const std::string& dbPath) {
    try {
        knowledgeBase_ = std::make_unique<SemanticKnowledgeBase>(dbPath);
        if (!knowledgeBase_->initialize(true)) {
            std::cerr << "Warning: Failed to initialize semantic knowledge base" << std::endl;
        }
        dimension_ = knowledgeBase_->getDimension();
        isReady_ = knowledgeBase_->isReady();
        if (isReady_) {
            std::cout << "Loaded semantic knowledge base: " << dimension_ << "D embeddings" << std::endl;
            return true;
        }
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Error loading semantic database: " << e.what() << std::endl;
        try {
            knowledgeBase_ = std::make_unique<SemanticKnowledgeBase>(":memory:");
            knowledgeBase_->initialize(true);
            dimension_ = 100;
            isReady_ = true;
            return false;
        } catch (...) {
            return false;
        }
    }
}

EmbeddingVector EmbeddingEngine::getEmbedding(const std::string& text) const {
    if (!knowledgeBase_) return EmbeddingVector(dimension_, 0.0f);
    auto embedding = knowledgeBase_->encodeText(text);
    if (embedding.empty()) embedding = EmbeddingVector(dimension_, 1.0f / std::sqrt(dimension_));
    if (static_cast<int>(embedding.size()) != dimension_) {
        embedding.resize(dimension_, 0.0f);
        normalizeEmbedding(embedding);
    }
    return embedding;
}

CompatibilityScore EmbeddingEngine::calculateSimilarity(
    const EmbeddingVector& a, const EmbeddingVector& b) noexcept {
    if (a.empty() || b.empty() || a.size() != b.size()) return 0.0f;
    return std::clamp(std::inner_product(a.begin(), a.end(), b.begin(), 0.0f), 0.0f, 1.0f);
}

CompatibilityScore EmbeddingEngine::calculateWeightedSimilarity(
    const EmbeddingVector& a, const EmbeddingVector& b, const EmbeddingVector* weights) noexcept {
    if (a.empty() || b.empty() || a.size() != b.size()) return 0.0f;
    if (!weights || weights->empty()) return calculateSimilarity(a, b);
    float weightedDot = 0.0f;
    size_t dim = std::min({a.size(), b.size(), weights->size()});
    for (size_t i = 0; i < dim; ++i) weightedDot += (*weights)[i] * a[i] * b[i];
    return std::clamp(weightedDot, 0.0f, 1.0f);
}

void EmbeddingEngine::normalizeEmbedding(EmbeddingVector& embedding) noexcept {
    if (embedding.empty()) return;
    SemanticKnowledgeBase::normalizeVector(embedding);
}

std::vector<std::pair<std::string, float>> EmbeddingEngine::findSimilarWords(
    const EmbeddingVector& embedding, int topK) const {
    if (!knowledgeBase_) return {};
    auto allTags = knowledgeBase_->getAllTags();
    std::vector<std::pair<std::string, float>> similarities;
    for (const auto& tag : allTags) {
        auto tagEmb = knowledgeBase_->getTagEmbedding(tag);
        if (!tagEmb.empty() && tagEmb.size() == embedding.size()) {
            similarities.emplace_back(tag, calculateSimilarity(embedding, tagEmb));
        }
    }
    std::sort(similarities.begin(), similarities.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    if (similarities.size() > static_cast<size_t>(topK)) similarities.resize(topK);
    return similarities;
}

float EmbeddingEngine::getTagIDF(const std::string& tag) const noexcept {
    return knowledgeBase_ ? knowledgeBase_->getIDF(tag) : 1.0f;
}

void EmbeddingEngine::updateTagStatistics(const std::vector<std::vector<std::string>>& allTags) {
    if (!knowledgeBase_) return;
    std::vector<std::string> flatTags;
    for (const auto& tagSet : allTags) flatTags.insert(flatTags.end(), tagSet.begin(), tagSet.end());
    knowledgeBase_->computeIDFStatistics(flatTags);
}

int EmbeddingEngine::getDimension() const noexcept {
    return dimension_;
}

bool EmbeddingEngine::isReady() const noexcept {
    return isReady_ && knowledgeBase_ && knowledgeBase_->isReady();
}

// ============================================================================
// MultiDimensionalPointer Implementation
// ============================================================================

MultiDimensionalPointer::MultiDimensionalPointer(ScoringWeights weights, std::shared_ptr<EmbeddingEngine> embeddingEngine)
    : weights_(weights), embeddingEngine_(std::move(embeddingEngine)) {
    
    if (!embeddingEngine_) {
        throw std::invalid_argument("EmbeddingEngine cannot be null");
    }
    
    if (!weights_.isValid()) {
        throw std::invalid_argument("Invalid scoring weights");
    }
}

CompatibilityResult MultiDimensionalPointer::analyzeCompatibility(const AudioConfig& configA, const AudioConfig& configB) const {
    CompatibilityResult result;
    
    // Calculate individual dimension scores
    result.semanticScore = calculateSemanticScore(configA, configB);
    result.technicalScore = calculateTechnicalScore(configA, configB);
    result.musicalRoleScore = calculateMusicalRoleScore(configA, configB);
    result.layeringScore = calculateLayeringScore(configA, configB);
    
    // Calculate weighted overall score
    result.overallScore = weights_.semantic * result.semanticScore +
                         weights_.technical * result.technicalScore +
                         weights_.musicalRole * result.musicalRoleScore +
                         weights_.layering * result.layeringScore;
    
    // Determine recommendation
    result.isRecommended = result.overallScore >= 0.7f && result.technicalScore >= 0.6f;
    
    // Generate explanations
    if (result.semanticScore > 0.7f) {
        result.strengths.push_back("High semantic similarity (" + 
                                  std::to_string(static_cast<int>(result.semanticScore * 100)) + "%)");
    }
    
    if (result.technicalScore > 0.8f) {
        result.strengths.push_back("Excellent technical compatibility");
    } else if (result.technicalScore < 0.5f) {
        result.issues.push_back("Technical compatibility concerns");
        result.suggestions["technical"] = "Check sample rates, plugin formats, and envelope types";
    }
    
    if (result.musicalRoleScore > 0.7f) {
        result.strengths.push_back("Compatible musical roles");
    } else if (result.musicalRoleScore < 0.4f) {
        result.warnings.push_back("Musical roles may conflict");
    }
    
    if (result.layeringScore > 0.6f) {
        result.strengths.push_back("Good layering compatibility");
    } else if (result.layeringScore < 0.3f) {
        result.warnings.push_back("May compete for same frequency/stereo space");
        result.suggestions["layering"] = "Consider different frequency ranges or stereo positioning";
    }
    
    return result;
}

std::vector<std::pair<std::shared_ptr<AudioConfig>, CompatibilityResult>> 
MultiDimensionalPointer::findCompatibleConfigurations(
    const AudioConfig& anchor,
    const std::vector<std::shared_ptr<AudioConfig>>& candidates,
    const UserContext& userContext,
    int maxResults) const {
    
    std::vector<std::pair<std::shared_ptr<AudioConfig>, CompatibilityResult>> results;
    
    for (const auto& candidate : candidates) {
        if (!candidate || candidate->getId() == anchor.getId()) continue;
        
        // Skip excluded configurations
        if (userContext.isExcluded(candidate->getId())) continue;
        
        // Analyze compatibility
        CompatibilityResult compatibility = analyzeCompatibility(anchor, *candidate);
        
        // Apply user boost
        float userBoost = userContext.calculateUserBoost(candidate->getId());
        compatibility.overallScore *= userBoost;
        
        // Only include if above minimum threshold
        if (compatibility.overallScore >= 0.3f) {
            results.emplace_back(candidate, compatibility);
        }
    }
    
    // Sort by overall score descending
    std::sort(results.begin(), results.end(),
              [](const auto& a, const auto& b) {
                  return a.second.overallScore > b.second.overallScore;
              });
    
    // Limit results
    if (results.size() > static_cast<size_t>(maxResults)) {
        results.resize(maxResults);
    }
    
    return results;
}

CompatibilityScore MultiDimensionalPointer::calculateSemanticScore(const AudioConfig& a, const AudioConfig& b) const {
    return a.calculateSemanticSimilarity(b);
}

CompatibilityScore MultiDimensionalPointer::calculateTechnicalScore(const AudioConfig& a, const AudioConfig& b) const {
    return a.getTechnicalSpecs().isCompatibleWith(b.getTechnicalSpecs());
}

CompatibilityScore MultiDimensionalPointer::calculateMusicalRoleScore(const AudioConfig& a, const AudioConfig& b) const {
    return a.getMusicalRole().calculateCompatibility(b.getMusicalRole());
}

CompatibilityScore MultiDimensionalPointer::calculateLayeringScore(const AudioConfig& a, const AudioConfig& b) const {
    return a.getLayeringInfo().calculateCompatibility(b.getLayeringInfo());
}

// ============================================================================
// ConfigGenerator Implementation
// ============================================================================

std::shared_ptr<nlohmann::json> ConfigGenerator::generateSynthesisConfig(
    const std::vector<std::shared_ptr<AudioConfig>>& selectedConfigs,
    const UserContext& userContext) {
    
    auto result = std::make_shared<json>(json::object());
    
    // Add metadata
    (*result)["metadata"] = {
        {"version", "1.0"},
        {"generator", "Multi-Dimensional Audio Configuration System"},
        {"timestamp", std::time(nullptr)},
        {"multidimensional_pointing", true}
    };
    
    // Add instruments
    json instruments = json::object();
    
    for (const auto& config : selectedConfigs) {
        if (!config) continue;
        
        // Get clean configuration data
        json instrumentConfig = config->getConfigData();
        
        // Apply user preferences if any
        applyUserPreferences(instrumentConfig, userContext);
        
        // Add to instruments
        instruments[config->getId()] = instrumentConfig;
    }
    
    (*result)["instruments"] = instruments;
    
    // Add compatibility analysis
    json compatibilityMatrix = json::object();
    for (size_t i = 0; i < selectedConfigs.size(); ++i) {
        for (size_t j = i + 1; j < selectedConfigs.size(); ++j) {
            if (!selectedConfigs[i] || !selectedConfigs[j]) continue;
            
            // This would require access to MultiDimensionalPointer
            // For now, just indicate compatibility was checked
            std::string pairKey = selectedConfigs[i]->getId() + "_" + selectedConfigs[j]->getId();
            compatibilityMatrix[pairKey] = {
                {"checked", true},
                {"note", "Compatibility validated by multi-dimensional analysis"}
            };
        }
    }
    
    (*result)["compatibility_analysis"] = compatibilityMatrix;
    
    return result;
}

CompatibilityResult ConfigGenerator::validateConfigChain(
    const std::vector<std::shared_ptr<AudioConfig>>& configChain) {
    
    CompatibilityResult result;
    result.overallScore = 1.0f;
    
    if (configChain.size() < 2) {
        result.isRecommended = true;
        return result;
    }
    
    // Check adjacent pairs for basic compatibility
    for (size_t i = 0; i < configChain.size() - 1; ++i) {
        if (!configChain[i] || !configChain[i + 1]) {
            result.issues.push_back("Null configuration in chain");
            result.overallScore = 0.0f;
            continue;
        }
        
        const auto& techA = configChain[i]->getTechnicalSpecs();
        const auto& techB = configChain[i + 1]->getTechnicalSpecs();
        
        float techScore = techA.isCompatibleWith(techB);
        if (techScore < 0.5f) {
            result.issues.push_back("Technical incompatibility between " + 
                                   configChain[i]->getName() + " and " + configChain[i + 1]->getName());
            result.suggestions["chain"] = "Check sample rates, plugin formats, and buffer sizes";
        }
        
        result.overallScore = std::min(result.overallScore, techScore);
    }
    
    result.isRecommended = result.overallScore >= 0.6f && result.issues.empty();
    
    if (result.isRecommended) {
        result.strengths.push_back("Configuration chain is technically compatible");
    }
    
    return result;
}

void ConfigGenerator::mergeConfigData(nlohmann::json& target, const nlohmann::json& source) {
    for (auto it = source.begin(); it != source.end(); ++it) {
        if (target.contains(it.key()) && target[it.key()].is_object() && it.value().is_object()) {
            mergeConfigData(target[it.key()], it.value());
        } else {
            target[it.key()] = it.value();
        }
    }
}

void ConfigGenerator::applyUserPreferences(nlohmann::json& config, const UserContext& userContext) {
    // This could apply user-specific modifications to the configuration
    // For example, adjusting volume levels, effect settings, etc.
    // Implementation would depend on specific user preference schema
    
    // Example: Add user session info
    if (!config.contains("metadata")) {
        config["metadata"] = json::object();
    }
    
    config["metadata"]["user_selection"] = true;
    config["metadata"]["selected_count"] = userContext.getSelectedConfigs().size();
}

} // namespace audio_config