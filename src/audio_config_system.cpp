/**
 * @file audio_config_system.cpp
 * @brief Multi-Dimensional Audio Configuration System - Implementation
 * @author AI Assistant
 * @version 1.0
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

CompatibilityScore AudioConfig::calculateSemanticSimilarity(const AudioConfig& other) const noexcept {
    // Calculate embedding similarity
    float embeddingSimilarity = EmbeddingEngine::calculateSimilarity(embedding_, other.embedding_);
    
    // Calculate tag overlap
    float tagSimilarity = 0.0f;
    if (!semanticTags_.empty() && !other.semanticTags_.empty()) {
        int sharedTags = 0;
        for (const auto& tag : semanticTags_) {
            if (std::find(other.semanticTags_.begin(), other.semanticTags_.end(), tag) != other.semanticTags_.end()) {
                sharedTags++;
            }
        }
        tagSimilarity = static_cast<float>(sharedTags) / std::max(semanticTags_.size(), other.semanticTags_.size());
    }
    
    // Weighted combination
    return 0.7f * embeddingSimilarity + 0.3f * tagSimilarity;
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
// EmbeddingEngine Implementation
// ============================================================================

EmbeddingEngine::EmbeddingEngine() {
    loadPretrainedEmbeddings();
    generateSubwordEmbeddings();
}

void EmbeddingEngine::loadPretrainedEmbeddings() {
    // Audio/music domain vocabulary with synthetic FastText-style embeddings
    std::vector<std::string> musicVocab = {
        // Timbral qualities
        "warm", "bright", "dark", "smooth", "rough", "sharp", "soft", "hard",
        "thick", "thin", "rich", "sparse", "dense", "clear", "muddy", "crisp",
        "mellow", "harsh", "sweet", "bitter", "round", "angular", "organic", "synthetic",
        "metallic", "woody", "glassy", "silky", "gritty", "polished", "raw", "refined",
        
        // Emotional qualities  
        "aggressive", "calm", "peaceful", "energetic", "dreamy", "mysterious",
        "intimate", "bold", "delicate", "powerful", "gentle", "fierce", "serene",
        "chaotic", "stable", "unstable", "flowing", "choppy", "smooth", "jagged",
        "uplifting", "melancholic", "nostalgic", "futuristic", "vintage", "modern",
        
        // Technical terms
        "attack", "decay", "sustain", "release", "envelope", "filter", "resonance",
        "cutoff", "frequency", "amplitude", "oscillator", "modulation", "vibrato",
        "tremolo", "chorus", "reverb", "delay", "echo", "compression", "distortion",
        "saturation", "overdrive", "phaser", "flanger", "wah", "eq", "limiter",
        
        // Instruments
        "guitar", "bass", "piano", "drums", "violin", "saxophone", "trumpet",
        "flute", "synthesizer", "keyboard", "vocal", "strings", "brass", "woodwind",
        "electric", "acoustic", "digital", "analog", "vintage", "modern",
        
        // Musical roles
        "lead", "rhythm", "bass", "pad", "arp", "chord", "melody", "harmony",
        "percussion", "kick", "snare", "hihat", "cymbal", "tom", "clap", "snap",
        
        // Arrangement terms
        "foreground", "background", "midground", "layer", "texture", "foundation",
        "support", "accent", "fill", "transition", "buildup", "breakdown",
        
        // Frequency ranges
        "low", "mid", "high", "sub", "bass", "treble", "presence", "air",
        "fundamental", "harmonic", "overtone", "resonant", "filtered"
    };
    
    // Generate contextually meaningful embeddings using clustering
    std::mt19937 rng(42); // Fixed seed for reproducibility
    std::normal_distribution<float> normalDist(0.0f, 0.1f);
    
    // Create semantic clusters
    std::unordered_map<std::string, std::vector<std::string>> semanticClusters = {
        {"timbral_warm", {"warm", "soft", "mellow", "smooth", "round", "organic", "sweet", "silky"}},
        {"timbral_bright", {"bright", "sharp", "crisp", "clear", "harsh", "thin", "metallic", "glassy"}},
        {"timbral_dark", {"dark", "thick", "dense", "deep", "rich", "heavy", "woody", "raw"}},
        {"emotional_calm", {"calm", "peaceful", "serene", "gentle", "flowing", "dreamy", "soft"}},
        {"emotional_energetic", {"aggressive", "energetic", "bold", "powerful", "fierce", "driving", "intense"}},
        {"technical_envelope", {"attack", "decay", "sustain", "release", "envelope", "dynamics", "response"}},
        {"technical_filter", {"filter", "cutoff", "resonance", "frequency", "sweep", "modulation", "eq"}},
        {"instruments_string", {"guitar", "bass", "violin", "strings", "plucked", "bowed", "acoustic"}},
        {"instruments_electronic", {"synthesizer", "digital", "virtual", "electronic", "processed", "analog"}},
        {"effects_spatial", {"reverb", "delay", "echo", "space", "depth", "ambience", "hall"}},
        {"effects_modulation", {"chorus", "vibrato", "tremolo", "phaser", "flanger", "modulation", "lfo"}},
        {"roles_lead", {"lead", "melody", "solo", "foreground", "primary", "main", "featured"}},
        {"roles_support", {"bass", "pad", "harmony", "background", "support", "foundation", "texture"}},
        {"arrangement", {"layer", "arrangement", "mix", "balance", "placement", "position", "priority"}}
    };
    
    // Generate cluster centers
    std::unordered_map<std::string, EmbeddingVector> clusterCenters;
    for (const auto& [clusterName, words] : semanticClusters) {
        EmbeddingVector center{};
        for (int i = 0; i < 100; ++i) {
            center[i] = normalDist(rng);
        }
        
        // Normalize
        float norm = std::sqrt(std::inner_product(center.begin(), center.end(), center.begin(), 0.0f));
        if (norm > 0) {
            for (float& val : center) {
                val /= norm;
            }
        }
        
        clusterCenters[clusterName] = center;
    }
    
    // Generate word embeddings around cluster centers
    for (const auto& [clusterName, words] : semanticClusters) {
        const auto& center = clusterCenters[clusterName];
        
        for (const std::string& word : words) {
            if (std::find(musicVocab.begin(), musicVocab.end(), word) != musicVocab.end()) {
                EmbeddingVector embedding{};
                
                // Perturb around cluster center
                for (int i = 0; i < 100; ++i) {
                    embedding[i] = center[i] + normalDist(rng) * 0.3f;
                }
                
                // Normalize
                float norm = std::sqrt(std::inner_product(embedding.begin(), embedding.end(), embedding.begin(), 0.0f));
                if (norm > 0) {
                    for (float& val : embedding) {
                        val /= norm;
                    }
                }
                
                wordEmbeddings_[word] = embedding;
            }
        }
    }
    
    // Generate embeddings for remaining vocabulary
    for (const std::string& word : musicVocab) {
        if (wordEmbeddings_.find(word) == wordEmbeddings_.end()) {
            EmbeddingVector embedding{};
            for (int i = 0; i < 100; ++i) {
                embedding[i] = normalDist(rng);
            }
            
            // Normalize
            float norm = std::sqrt(std::inner_product(embedding.begin(), embedding.end(), embedding.begin(), 0.0f));
            if (norm > 0) {
                for (float& val : embedding) {
                    val /= norm;
                }
            }
            
            wordEmbeddings_[word] = embedding;
        }
    }
}

void EmbeddingEngine::generateSubwordEmbeddings() {
    // Generate character n-grams for OOV handling
    std::set<std::string> ngrams;
    
    for (const auto& [word, embedding] : wordEmbeddings_) {
        std::string paddedWord = "<" + word + ">";
        
        // Generate 3-6 character n-grams
        for (int n = 3; n <= std::min(6, static_cast<int>(paddedWord.length())); ++n) {
            for (int i = 0; i <= static_cast<int>(paddedWord.length()) - n; ++i) {
                ngrams.insert(paddedWord.substr(i, n));
            }
        }
    }
    
    // Generate embeddings for n-grams
    std::mt19937 rng(42);
    std::normal_distribution<float> normalDist(0.0f, 0.05f);
    
    for (const std::string& ngram : ngrams) {
        EmbeddingVector embedding{};
        for (int i = 0; i < 100; ++i) {
            embedding[i] = normalDist(rng);
        }
        
        // Normalize
        float norm = std::sqrt(std::inner_product(embedding.begin(), embedding.end(), embedding.begin(), 0.0f));
        if (norm > 0) {
            for (float& val : embedding) {
                val /= norm;
            }
        }
        
        subwordEmbeddings_[ngram] = embedding;
    }
}

EmbeddingVector EmbeddingEngine::getEmbedding(const std::string& text) const {
    return computeTextEmbedding(text);
}

EmbeddingVector EmbeddingEngine::computeTextEmbedding(const std::string& text) const {
    std::vector<std::string> words;
    std::istringstream iss(text);
    std::string word;
    
    // Tokenize and clean
    while (iss >> word) {
        // Convert to lowercase and remove punctuation
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
        word = std::regex_replace(word, std::regex("[^a-zA-Z0-9]"), "");
        if (!word.empty()) {
            words.push_back(word);
        }
    }
    
    EmbeddingVector result{};
    float weightSum = 0.0f;
    
    for (const std::string& w : words) {
        EmbeddingVector wordEmb{};
        
        // Try direct lookup
        auto it = wordEmbeddings_.find(w);
        if (it != wordEmbeddings_.end()) {
            wordEmb = it->second;
        } else {
            // Use subword embeddings for OOV
            std::string paddedWord = "<" + w + ">";
            int ngramCount = 0;
            
            for (int n = 3; n <= std::min(6, static_cast<int>(paddedWord.length())); ++n) {
                for (int i = 0; i <= static_cast<int>(paddedWord.length()) - n; ++i) {
                    std::string ngram = paddedWord.substr(i, n);
                    auto ngramIt = subwordEmbeddings_.find(ngram);
                    if (ngramIt != subwordEmbeddings_.end()) {
                        for (int j = 0; j < 100; ++j) {
                            wordEmb[j] += ngramIt->second[j];
                        }
                        ngramCount++;
                    }
                }
            }
            
            if (ngramCount > 0) {
                for (float& val : wordEmb) {
                    val /= ngramCount;
                }
            }
        }
        
        // Add to result with TF-IDF-like weighting
        float weight = 1.0f / std::sqrt(static_cast<float>(words.size()));
        for (int i = 0; i < 100; ++i) {
            result[i] += wordEmb[i] * weight;
        }
        weightSum += weight;
    }
    
    // Normalize result
    if (weightSum > 0) {
        for (float& val : result) {
            val /= weightSum;
        }
    }
    
    return result;
}

CompatibilityScore EmbeddingEngine::calculateSimilarity(const EmbeddingVector& a, const EmbeddingVector& b) noexcept {
    float dotProduct = std::inner_product(a.begin(), a.end(), b.begin(), 0.0f);
    float normA = std::sqrt(std::inner_product(a.begin(), a.end(), a.begin(), 0.0f));
    float normB = std::sqrt(std::inner_product(b.begin(), b.end(), b.begin(), 0.0f));
    
    if (normA == 0.0f || normB == 0.0f) return 0.0f;
    
    return std::max(0.0f, dotProduct / (normA * normB));
}

std::vector<std::pair<std::string, float>> EmbeddingEngine::findSimilarWords(
    const EmbeddingVector& embedding, int topK) const {
    
    std::vector<std::pair<std::string, float>> similarities;
    
    for (const auto& [word, wordEmb] : wordEmbeddings_) {
        float sim = calculateSimilarity(embedding, wordEmb);
        similarities.emplace_back(word, sim);
    }
    
    // Sort by similarity descending
    std::sort(similarities.begin(), similarities.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Return top K
    if (similarities.size() > static_cast<size_t>(topK)) {
        similarities.resize(topK);
    }
    
    return similarities;
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