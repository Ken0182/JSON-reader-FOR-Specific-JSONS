#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <set>
#include <memory>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <fstream>
#include <sstream>
#include <random>
#include <chrono>
#include <iomanip>
#include <functional>
#include <queue>
#include <regex>
#include <ctime>

/**
 * @file integrated_ai_synthesis_system.cpp
 * @brief Integrated AI-driven instrument synthesis system with FastText embeddings and semantic indexing
 * @author AI Synthesis Team
 * @version 2.0
 */

namespace AISynthesis {

// ============================================================================
// CORE TYPE DEFINITIONS AND UTILITIES
// ============================================================================

using EmbeddingVector = std::vector<float>;
using ConfigurationId = std::string;

/**
 * @brief Utility functions for vector operations
 */
class VectorUtils {
public:
    /**
     * @brief Compute cosine similarity between two vectors
     */
    static float cosineSimilarity(const EmbeddingVector& a, const EmbeddingVector& b) {
        if (a.size() != b.size() || a.empty()) return 0.0f;
        
        float dotProduct = 0.0f;
        float normA = 0.0f;
        float normB = 0.0f;
        
        for (size_t i = 0; i < a.size(); ++i) {
            dotProduct += a[i] * b[i];
            normA += a[i] * a[i];
            normB += b[i] * b[i];
        }
        
        if (normA == 0.0f || normB == 0.0f) return 0.0f;
        
        return dotProduct / (std::sqrt(normA) * std::sqrt(normB));
    }
    
    /**
     * @brief Normalize vector to unit length
     */
    static EmbeddingVector normalize(const EmbeddingVector& vec) {
        float norm = 0.0f;
        for (float val : vec) {
            norm += val * val;
        }
        norm = std::sqrt(norm);
        
        if (norm == 0.0f) return vec;
        
        EmbeddingVector normalized;
        normalized.reserve(vec.size());
        for (float val : vec) {
            normalized.push_back(val / norm);
        }
        return normalized;
    }
    
    /**
     * @brief Compute Euclidean distance between vectors
     */
    static float euclideanDistance(const EmbeddingVector& a, const EmbeddingVector& b) {
        if (a.size() != b.size()) return std::numeric_limits<float>::max();
        
        float sum = 0.0f;
        for (size_t i = 0; i < a.size(); ++i) {
            float diff = a[i] - b[i];
            sum += diff * diff;
        }
        return std::sqrt(sum);
    }
    
    /**
     * @brief Average multiple embedding vectors
     */
    static EmbeddingVector averageVectors(const std::vector<EmbeddingVector>& vectors) {
        if (vectors.empty()) return {};
        
        size_t dim = vectors[0].size();
        EmbeddingVector result(dim, 0.0f);
        
        for (const auto& vec : vectors) {
            if (vec.size() != dim) continue;
            for (size_t i = 0; i < dim; ++i) {
                result[i] += vec[i];
            }
        }
        
        float count = static_cast<float>(vectors.size());
        for (float& val : result) {
            val /= count;
        }
        
        return normalize(result);
    }
};

/**
 * @brief Text preprocessing utilities
 */
class TextUtils {
public:
    /**
     * @brief Convert text to lowercase
     */
    static std::string toLowerCase(const std::string& text) {
        std::string result = text;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }
    
    /**
     * @brief Split text into tokens
     */
    static std::vector<std::string> tokenize(const std::string& text) {
        std::vector<std::string> tokens;
        std::istringstream iss(text);
        std::string token;
        
        while (iss >> token) {
            // Remove punctuation
            token.erase(std::remove_if(token.begin(), token.end(), 
                [](char c) { return std::ispunct(c); }), token.end());
            
            if (!token.empty()) {
                tokens.push_back(toLowerCase(token));
            }
        }
        
        return tokens;
    }
    
    /**
     * @brief Clean text for processing
     */
    static std::string cleanText(const std::string& text) {
        std::string cleaned = text;
        
        // Replace multiple spaces with single space
        std::regex multiSpace("\\s+");
        cleaned = std::regex_replace(cleaned, multiSpace, " ");
        
        // Trim whitespace
        size_t start = cleaned.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) {
            return "";
        }
        size_t end = cleaned.find_last_not_of(" \t\n\r");
        cleaned = cleaned.substr(start, end - start + 1);
        
        return cleaned;
    }
};

// ============================================================================
// FASTTEXT EMBEDDING ENGINE
// ============================================================================

/**
 * @brief FastText-style embedding engine with subword modeling
 */
class FastTextEmbeddingEngine {
private:
    std::unordered_map<std::string, EmbeddingVector> wordEmbeddings_;
    std::unordered_map<std::string, EmbeddingVector> subwordEmbeddings_;
    size_t embeddingDimension_;
    size_t minSubwordLength_;
    size_t maxSubwordLength_;
    std::mt19937 rng_;
    
         /**
      * @brief Generate subwords for a given word
      */
     std::vector<std::string> generateSubwords(const std::string& word) const {
         std::vector<std::string> subwords;
         std::string paddedWord = "<" + word + ">";
         
         for (size_t len = minSubwordLength_; len <= maxSubwordLength_ && len <= paddedWord.length(); ++len) {
             if (paddedWord.length() >= len) {
                 for (size_t i = 0; i <= paddedWord.length() - len; ++i) {
                     subwords.push_back(paddedWord.substr(i, len));
                 }
             }
         }
         
         return subwords;
     }
    
    /**
     * @brief Generate random embedding vector
     */
    EmbeddingVector generateRandomEmbedding() {
        std::uniform_real_distribution<float> dist(-0.1f, 0.1f);
        EmbeddingVector embedding;
        embedding.reserve(embeddingDimension_);
        
        for (size_t i = 0; i < embeddingDimension_; ++i) {
            embedding.push_back(dist(rng_));
        }
        
        return VectorUtils::normalize(embedding);
    }
    
    /**
     * @brief Get or create word embedding
     */
    EmbeddingVector getWordEmbedding(const std::string& word) {
        auto it = wordEmbeddings_.find(word);
        if (it != wordEmbeddings_.end()) {
            return it->second;
        }
        
        // Generate embedding from subwords
        std::vector<std::string> subwords = generateSubwords(word);
        std::vector<EmbeddingVector> subwordVecs;
        
        for (const std::string& subword : subwords) {
            auto subIt = subwordEmbeddings_.find(subword);
            if (subIt == subwordEmbeddings_.end()) {
                subwordEmbeddings_[subword] = generateRandomEmbedding();
                subIt = subwordEmbeddings_.find(subword);
            }
            subwordVecs.push_back(subIt->second);
        }
        
        EmbeddingVector wordEmbedding = VectorUtils::averageVectors(subwordVecs);
        wordEmbeddings_[word] = wordEmbedding;
        return wordEmbedding;
    }

public:
    /**
     * @brief Constructor
     */
    explicit FastTextEmbeddingEngine(size_t dimension = 100, size_t minSubword = 3, size_t maxSubword = 6)
        : embeddingDimension_(dimension)
        , minSubwordLength_(minSubword)
        , maxSubwordLength_(maxSubword)
        , rng_(std::chrono::steady_clock::now().time_since_epoch().count()) {
        
        // Initialize with common audio synthesis terms
        initializeAudioVocabulary();
    }
    
    /**
     * @brief Initialize with audio synthesis vocabulary
     */
    void initializeAudioVocabulary() {
        std::vector<std::string> audioTerms = {
            "warm", "bright", "dark", "lush", "gritty", "ethereal", "vintage",
            "organic", "synthetic", "metallic", "wooden", "glass", "plastic",
            "calm", "energetic", "nostalgic", "aggressive", "dreamy", "playful",
            "punchy", "smooth", "percussive", "sustained", "rhythmic", "tribal",
            "guitar", "synthesizer", "bass", "drums", "piano", "strings",
            "reverb", "delay", "distortion", "chorus", "flanger", "phaser",
            "attack", "decay", "sustain", "release", "cutoff", "resonance",
            "oscillator", "filter", "envelope", "modulation", "frequency"
        };
        
        // Pre-generate embeddings for common terms
        for (const std::string& term : audioTerms) {
            getWordEmbedding(term);
        }
    }
    
    /**
     * @brief Get sentence embedding by averaging word embeddings
     */
    EmbeddingVector getSentenceEmbedding(const std::string& sentence) {
        std::vector<std::string> words = TextUtils::tokenize(sentence);
        if (words.empty()) {
            return EmbeddingVector(embeddingDimension_, 0.0f);
        }
        
        std::vector<EmbeddingVector> wordVecs;
        for (const std::string& word : words) {
            wordVecs.push_back(getWordEmbedding(word));
        }
        
        return VectorUtils::averageVectors(wordVecs);
    }
    
    /**
     * @brief Compute similarity between two texts
     */
    float computeTextSimilarity(const std::string& text1, const std::string& text2) {
        EmbeddingVector emb1 = getSentenceEmbedding(text1);
        EmbeddingVector emb2 = getSentenceEmbedding(text2);
        return VectorUtils::cosineSimilarity(emb1, emb2);
    }
    
    /**
     * @brief Find most similar words to a given word
     */
    std::vector<std::pair<std::string, float>> findSimilarWords(const std::string& word, size_t topK = 10) {
        EmbeddingVector targetEmbedding = getWordEmbedding(word);
        
        std::vector<std::pair<std::string, float>> similarities;
        for (const auto& [w, embedding] : wordEmbeddings_) {
            if (w != word) {
                float similarity = VectorUtils::cosineSimilarity(targetEmbedding, embedding);
                similarities.emplace_back(w, similarity);
            }
        }
        
        std::partial_sort(similarities.begin(), 
                         similarities.begin() + std::min(topK, similarities.size()),
                         similarities.end(),
                         [](const auto& a, const auto& b) { return a.second > b.second; });
        
        similarities.resize(std::min(topK, similarities.size()));
        return similarities;
    }
    
    /**
     * @brief Train on text corpus (simplified version)
     */
    void trainOnCorpus(const std::vector<std::string>& corpus) {
        std::cout << "Training on corpus of " << corpus.size() << " documents...\n";
        
        for (const std::string& document : corpus) {
            std::vector<std::string> words = TextUtils::tokenize(document);
            for (const std::string& word : words) {
                getWordEmbedding(word); // This will create embeddings for new words
            }
        }
        
        std::cout << "Training complete. Vocabulary size: " << wordEmbeddings_.size() << "\n";
    }
    
    /**
     * @brief Get embedding dimension
     */
    size_t getDimension() const { return embeddingDimension_; }
    
    /**
     * @brief Get vocabulary size
     */
    size_t getVocabularySize() const { return wordEmbeddings_.size(); }
    
    /**
     * @brief Save embeddings to file
     */
    bool saveEmbeddings(const std::string& filename) const {
        std::ofstream file(filename, std::ios::binary);
        if (!file) return false;
        
        // Write header
        size_t vocabSize = wordEmbeddings_.size();
        file.write(reinterpret_cast<const char*>(&vocabSize), sizeof(vocabSize));
        file.write(reinterpret_cast<const char*>(&embeddingDimension_), sizeof(embeddingDimension_));
        
        // Write embeddings
        for (const auto& [word, embedding] : wordEmbeddings_) {
            size_t wordLen = word.length();
            file.write(reinterpret_cast<const char*>(&wordLen), sizeof(wordLen));
            file.write(word.c_str(), wordLen);
            file.write(reinterpret_cast<const char*>(embedding.data()), 
                      embedding.size() * sizeof(float));
        }
        
        return true;
    }
    
    /**
     * @brief Load embeddings from file
     */
    bool loadEmbeddings(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) return false;
        
        // Read header
        size_t vocabSize;
        size_t dimension;
        file.read(reinterpret_cast<char*>(&vocabSize), sizeof(vocabSize));
        file.read(reinterpret_cast<char*>(&dimension), sizeof(dimension));
        
        if (dimension != embeddingDimension_) {
            std::cerr << "Embedding dimension mismatch\n";
            return false;
        }
        
        wordEmbeddings_.clear();
        
        // Read embeddings
        for (size_t i = 0; i < vocabSize; ++i) {
            size_t wordLen;
            file.read(reinterpret_cast<char*>(&wordLen), sizeof(wordLen));
            
            std::string word(wordLen, '\0');
            file.read(&word[0], wordLen);
            
            EmbeddingVector embedding(embeddingDimension_);
            file.read(reinterpret_cast<char*>(embedding.data()), 
                     embedding.size() * sizeof(float));
            
            wordEmbeddings_[word] = embedding;
        }
        
        return true;
    }
};

// ============================================================================
// ENHANCED SEMANTIC DATABASE
// ============================================================================

/**
 * @brief Enhanced semantic database with FastText embeddings
 */
class EnhancedSemanticDatabase {
public:
    struct SemanticEntry {
        std::string keyword;
        std::string category;
        std::vector<std::string> aliases;
        float scoreWeight;
        std::string description;
        EmbeddingVector embedding;
        
        SemanticEntry() : scoreWeight(1.0f) {}
        
        SemanticEntry(const std::string& kw, const std::string& cat, 
                     const std::vector<std::string>& al, float weight, 
                     const std::string& desc)
            : keyword(kw), category(cat), aliases(al), scoreWeight(weight), description(desc) {}
    };

private:
    std::unordered_map<std::string, SemanticEntry> semanticEntries_;
    std::unordered_map<std::string, std::vector<std::string>> categoryIndex_;
    FastTextEmbeddingEngine& embeddingEngine_;
    
public:
    /**
     * @brief Constructor
     */
    explicit EnhancedSemanticDatabase(FastTextEmbeddingEngine& engine) 
        : embeddingEngine_(engine) {
        initializeDefaultEntries();
    }
    
    /**
     * @brief Initialize with default semantic entries
     */
    void initializeDefaultEntries() {
        // Timbral characteristics
        addEntry("warm", "timbral", {"soft", "mellow", "cozy", "gentle"}, 0.9f, 
                "Soft, enveloping timbral characteristic");
        addEntry("bright", "timbral", {"shiny", "clear", "brilliant", "luminous"}, 0.85f,
                "Clear, high-frequency emphasized timbre");
        addEntry("dark", "timbral", {"deep", "shadowy", "mysterious", "brooding"}, 0.8f,
                "Low-frequency emphasized, subdued timbre");
        addEntry("lush", "timbral", {"rich", "full", "luxurious", "dense"}, 0.85f,
                "Rich, harmonically complex timbre");
        addEntry("gritty", "timbral", {"rough", "distorted", "harsh", "raw"}, 0.8f,
                "Rough, distorted timbral quality");
        addEntry("ethereal", "timbral", {"airy", "heavenly", "floating", "weightless"}, 0.8f,
                "Light, floating timbral characteristic");
        
        // Emotional characteristics
        addEntry("calm", "emotional", {"peaceful", "relaxed", "serene", "tranquil"}, 0.8f,
                "Peaceful, relaxing emotional quality");
        addEntry("energetic", "emotional", {"lively", "vibrant", "dynamic", "exciting"}, 0.9f,
                "High-energy, exciting emotional characteristic");
        addEntry("nostalgic", "emotional", {"sentimental", "bittersweet", "wistful", "longing"}, 0.95f,
                "Nostalgic, sentimental emotional quality");
        addEntry("aggressive", "emotional", {"intense", "fierce", "powerful", "forceful"}, 0.9f,
                "Intense, aggressive emotional characteristic");
        addEntry("dreamy", "emotional", {"ethereal", "surreal", "floating", "ambient"}, 0.85f,
                "Dreamy, ambient emotional quality");
        
        // Dynamic characteristics
        addEntry("punchy", "dynamic", {"sharp", "impactful", "snappy", "crisp"}, 0.85f,
                "Sharp, impactful dynamic characteristic");
        addEntry("smooth", "dynamic", {"flowing", "seamless", "fluid", "graceful"}, 0.8f,
                "Smooth, flowing dynamic quality");
        addEntry("percussive", "dynamic", {"strike", "hit", "attack", "transient"}, 0.7f,
                "Percussive, attack-heavy dynamic characteristic");
        
        // Material characteristics
        addEntry("organic", "material", {"natural", "acoustic", "real", "living"}, 0.8f,
                "Natural, organic material quality");
        addEntry("synthetic", "material", {"digital", "artificial", "electronic", "processed"}, 0.75f,
                "Synthetic, electronic material characteristic");
        addEntry("metallic", "material", {"metal", "steel", "iron", "hard"}, 0.75f,
                "Metallic, hard material characteristic");
        
        // Structural characteristics
        addEntry("intro", "structural", {"beginning", "opening", "start", "prelude"}, 0.8f,
                "Introduction section of musical structure");
        addEntry("verse", "structural", {"stanza", "main", "narrative", "story"}, 0.8f,
                "Verse section of musical structure");
        addEntry("chorus", "structural", {"refrain", "hook", "main-theme", "climax"}, 0.9f,
                "Chorus section of musical structure");
        
        std::cout << "Initialized semantic database with " << semanticEntries_.size() << " entries\n";
    }
    
    /**
     * @brief Add semantic entry
     */
    void addEntry(const std::string& keyword, const std::string& category,
                  const std::vector<std::string>& aliases, float scoreWeight,
                  const std::string& description) {
        SemanticEntry entry(keyword, category, aliases, scoreWeight, description);
        
        // Generate embedding for the keyword and its description
        std::string fullText = keyword + " " + description;
        for (const std::string& alias : aliases) {
            fullText += " " + alias;
        }
        entry.embedding = embeddingEngine_.getSentenceEmbedding(fullText);
        
        semanticEntries_[keyword] = entry;
        categoryIndex_[category].push_back(keyword);
    }
    
    /**
     * @brief Find semantic matches for a query
     */
    std::vector<std::pair<std::string, float>> findMatches(const std::string& query, 
                                                          size_t topK = 10,
                                                          float threshold = 0.0f) const {
        EmbeddingVector queryEmbedding = embeddingEngine_.getSentenceEmbedding(query);
        
        std::vector<std::pair<std::string, float>> matches;
        
        for (const auto& [keyword, entry] : semanticEntries_) {
            float similarity = VectorUtils::cosineSimilarity(queryEmbedding, entry.embedding);
            similarity *= entry.scoreWeight; // Apply weight
            
            if (similarity >= threshold) {
                matches.emplace_back(keyword, similarity);
            }
        }
        
        // Sort by similarity (descending)
        std::partial_sort(matches.begin(), 
                         matches.begin() + std::min(topK, matches.size()),
                         matches.end(),
                         [](const auto& a, const auto& b) { return a.second > b.second; });
        
        matches.resize(std::min(topK, matches.size()));
        return matches;
    }
    
    /**
     * @brief Get entry by keyword
     */
    const SemanticEntry* getEntry(const std::string& keyword) const {
        auto it = semanticEntries_.find(keyword);
        return (it != semanticEntries_.end()) ? &it->second : nullptr;
    }
    
    /**
     * @brief Get all entries in a category
     */
    std::vector<std::string> getCategory(const std::string& category) const {
        auto it = categoryIndex_.find(category);
        return (it != categoryIndex_.end()) ? it->second : std::vector<std::string>();
    }
    
    /**
     * @brief Get all categories
     */
    std::vector<std::string> getAllCategories() const {
        std::vector<std::string> categories;
        for (const auto& [category, _] : categoryIndex_) {
            categories.push_back(category);
        }
        return categories;
    }
    
    /**
     * @brief Get database size
     */
    size_t size() const { return semanticEntries_.size(); }
    
    /**
     * @brief Print database statistics
     */
    void printStatistics() const {
        std::cout << "\nSemantic Database Statistics:\n";
        std::cout << "Total entries: " << semanticEntries_.size() << "\n";
        std::cout << "Categories: " << categoryIndex_.size() << "\n";
        
        for (const auto& [category, keywords] : categoryIndex_) {
            std::cout << "  " << category << ": " << keywords.size() << " entries\n";
        }
        
        std::cout << "Embedding dimension: " << embeddingEngine_.getDimension() << "\n";
    }
};

// ============================================================================
// CONFIGURATION ENTRY SYSTEM
// ============================================================================

/**
 * @brief Instrument configuration entry with embedding support
 */
class ConfigurationEntry {
public:
    struct ParameterInfo {
        std::string name;
        std::string value;
        std::string description;
        
        ParameterInfo() = default;
        ParameterInfo(const std::string& n, const std::string& v, const std::string& d = "")
            : name(n), value(v), description(d) {}
    };

private:
    ConfigurationId id_;
    std::string name_;
    std::string instrumentType_;
    std::string description_;
    std::vector<std::string> tags_;
    std::vector<ParameterInfo> parameters_;
    EmbeddingVector contentEmbedding_;
    float userBoost_;
    bool isExcluded_;
    
public:
    /**
     * @brief Constructor
     */
    explicit ConfigurationEntry(const ConfigurationId& id = "",
                               const std::string& name = "",
                               const std::string& instrumentType = "",
                               const std::string& description = "")
        : id_(id), name_(name), instrumentType_(instrumentType), description_(description)
        , userBoost_(0.0f), isExcluded_(false) {}
    
    // Getters
    const ConfigurationId& getId() const { return id_; }
    const std::string& getName() const { return name_; }
    const std::string& getInstrumentType() const { return instrumentType_; }
    const std::string& getDescription() const { return description_; }
    const std::vector<std::string>& getTags() const { return tags_; }
    const std::vector<ParameterInfo>& getParameters() const { return parameters_; }
    const EmbeddingVector& getContentEmbedding() const { return contentEmbedding_; }
    float getUserBoost() const { return userBoost_; }
    bool isExcluded() const { return isExcluded_; }
    
    // Setters
    void setName(const std::string& name) { name_ = name; }
    void setInstrumentType(const std::string& type) { instrumentType_ = type; }
    void setDescription(const std::string& description) { description_ = description; }
    void setUserBoost(float boost) { userBoost_ = boost; }
    void setExcluded(bool excluded) { isExcluded_ = excluded; }
    
    /**
     * @brief Add tag
     */
    void addTag(const std::string& tag) {
        if (std::find(tags_.begin(), tags_.end(), tag) == tags_.end()) {
            tags_.push_back(tag);
        }
    }
    
    /**
     * @brief Add parameter
     */
    void addParameter(const std::string& name, const std::string& value, 
                     const std::string& description = "") {
        parameters_.emplace_back(name, value, description);
    }
    
    /**
     * @brief Generate content embedding using FastText engine
     */
    void generateEmbedding(FastTextEmbeddingEngine& engine) {
        std::string fullContent = name_ + " " + instrumentType_ + " " + description_;
        
        // Add tags
        for (const std::string& tag : tags_) {
            fullContent += " " + tag;
        }
        
        // Add parameter names and values
        for (const auto& param : parameters_) {
            fullContent += " " + param.name + " " + param.value + " " + param.description;
        }
        
        contentEmbedding_ = engine.getSentenceEmbedding(fullContent);
    }
    
    /**
     * @brief Get content as text for search
     */
    std::string getContentText() const {
        std::string content = name_ + " " + instrumentType_ + " " + description_;
        
        for (const std::string& tag : tags_) {
            content += " " + tag;
        }
        
        for (const auto& param : parameters_) {
            content += " " + param.name + " " + param.value + " " + param.description;
        }
        
        return content;
    }
    
    /**
     * @brief Compute similarity to query embedding
     */
    float computeSimilarity(const EmbeddingVector& queryEmbedding) const {
        float baseSimilarity = VectorUtils::cosineSimilarity(contentEmbedding_, queryEmbedding);
        return baseSimilarity + userBoost_;
    }
    
    /**
     * @brief Print configuration entry
     */
    void print() const {
        std::cout << "ID: " << id_ << "\n";
        std::cout << "Name: " << name_ << "\n";
        std::cout << "Type: " << instrumentType_ << "\n";
        std::cout << "Description: " << description_ << "\n";
        std::cout << "User Boost: " << std::fixed << std::setprecision(3) << userBoost_ << "\n";
        std::cout << "Excluded: " << (isExcluded_ ? "Yes" : "No") << "\n";
        
        if (!tags_.empty()) {
            std::cout << "Tags: ";
            for (size_t i = 0; i < tags_.size(); ++i) {
                std::cout << tags_[i];
                if (i < tags_.size() - 1) std::cout << ", ";
            }
            std::cout << "\n";
        }
        
        if (!parameters_.empty()) {
            std::cout << "Parameters:\n";
            for (const auto& param : parameters_) {
                std::cout << "  " << param.name << ": " << param.value;
                if (!param.description.empty()) {
                    std::cout << " (" << param.description << ")";
                }
                std::cout << "\n";
            }
        }
        std::cout << "\n";
    }
};

// ============================================================================
// POINTING INDEX SYSTEM
// ============================================================================

/**
 * @brief Enhanced pointing index system with FastText embeddings
 */
class PointingIndexSystem {
private:
    std::vector<std::unique_ptr<ConfigurationEntry>> configurations_;
    std::unordered_map<ConfigurationId, size_t> idToIndex_;
    FastTextEmbeddingEngine& embeddingEngine_;
    EnhancedSemanticDatabase& semanticDb_;
    
    // Search and ranking state
    std::vector<std::pair<size_t, float>> lastSearchResults_;
    std::string lastQuery_;
    
public:
    /**
     * @brief Constructor
     */
    explicit PointingIndexSystem(FastTextEmbeddingEngine& engine, 
                                EnhancedSemanticDatabase& semanticDb)
        : embeddingEngine_(engine), semanticDb_(semanticDb) {
        initializeDefaultConfigurations();
    }
    
    /**
     * @brief Initialize with default configurations
     */
    void initializeDefaultConfigurations() {
        // Guitar configurations
        auto guitar1 = std::make_unique<ConfigurationEntry>(
            "guitar_warm_acoustic", 
            "Warm Acoustic Guitar",
            "acoustic_guitar",
            "Warm, resonant acoustic guitar with natural wood character"
        );
        guitar1->addTag("warm");
        guitar1->addTag("acoustic");
        guitar1->addTag("organic");
        guitar1->addParameter("attack", "0.01", "Fast attack for plucked strings");
        guitar1->addParameter("decay", "1.2", "Natural decay time");
        guitar1->addParameter("cutoff", "3000", "Low-pass filter cutoff frequency");
        guitar1->addParameter("reverb", "0.3", "Room reverb amount");
        configurations_.push_back(std::move(guitar1));
        
        auto guitar2 = std::make_unique<ConfigurationEntry>(
            "guitar_bright_electric",
            "Bright Electric Guitar", 
            "electric_guitar",
            "Bright, cutting electric guitar with crisp highs"
        );
        guitar2->addTag("bright");
        guitar2->addTag("electric");
        guitar2->addTag("punchy");
        guitar2->addParameter("attack", "0.005", "Very fast attack");
        guitar2->addParameter("sustain", "0.8", "High sustain level");
        guitar2->addParameter("distortion", "0.4", "Moderate distortion");
        guitar2->addParameter("chorus", "0.2", "Light chorus effect");
        configurations_.push_back(std::move(guitar2));
        
        // Synthesizer configurations
        auto synth1 = std::make_unique<ConfigurationEntry>(
            "synth_lush_pad",
            "Lush Ambient Pad",
            "synthesizer_subtractive",
            "Rich, evolving pad with multiple oscillators and reverb"
        );
        synth1->addTag("lush");
        synth1->addTag("ambient");
        synth1->addTag("evolving");
        synth1->addTag("dreamy");
        synth1->addParameter("osc1_wave", "sawtooth", "Primary sawtooth oscillator");
        synth1->addParameter("osc2_wave", "square", "Secondary square wave");
        synth1->addParameter("filter_type", "lowpass", "Low-pass filter");
        synth1->addParameter("reverb", "0.8", "Heavy reverb");
        synth1->addParameter("attack", "2.0", "Slow attack for pad sound");
        configurations_.push_back(std::move(synth1));
        
        auto synth2 = std::make_unique<ConfigurationEntry>(
            "synth_aggressive_lead",
            "Aggressive Lead Synth",
            "synthesizer_subtractive", 
            "Cutting lead synthesizer with aggressive filtering and distortion"
        );
        synth2->addTag("aggressive");
        synth2->addTag("energetic");
        synth2->addTag("lead");
        synth2->addTag("cutting");
        synth2->addParameter("osc_wave", "sawtooth", "Sawtooth wave for brightness");
        synth2->addParameter("filter_cutoff", "2000", "Resonant filter sweep");
        synth2->addParameter("filter_res", "0.7", "High resonance");
        synth2->addParameter("distortion", "0.6", "Heavy distortion");
        synth2->addParameter("attack", "0.01", "Sharp attack");
        configurations_.push_back(std::move(synth2));
        
        // Bass configurations
        auto bass1 = std::make_unique<ConfigurationEntry>(
            "bass_deep_sub",
            "Deep Sub Bass",
            "synthesizer_bass",
            "Deep, rumbling sub bass with long sustain"
        );
        bass1->addTag("deep");
        bass1->addTag("sub");
        bass1->addTag("sustained");
        bass1->addTag("powerful");
        bass1->addParameter("osc_wave", "sine", "Pure sine wave for sub");
        bass1->addParameter("attack", "0.02", "Slight attack");
        bass1->addParameter("sustain", "1.0", "Full sustain");
        bass1->addParameter("lowpass", "200", "Very low cutoff");
        configurations_.push_back(std::move(bass1));
        
        auto bass2 = std::make_unique<ConfigurationEntry>(
            "bass_punchy_electric",
            "Punchy Electric Bass",
            "electric_bass",
            "Punchy electric bass with percussive attack and midrange presence"
        );
        bass2->addTag("punchy");
        bass2->addTag("percussive");
        bass2->addTag("electric");
        bass2->addTag("midrange");
        bass2->addParameter("attack", "0.001", "Very sharp attack");
        bass2->addParameter("decay", "0.3", "Quick decay");
        bass2->addParameter("compression", "0.6", "Moderate compression");
        bass2->addParameter("eq_mid", "0.3", "Midrange boost");
        configurations_.push_back(std::move(bass2));
        
        // Drum configurations
        auto drums1 = std::make_unique<ConfigurationEntry>(
            "drums_acoustic_kit",
            "Acoustic Drum Kit",
            "acoustic_drums",
            "Natural acoustic drum kit with room ambience"
        );
        drums1->addTag("acoustic");
        drums1->addTag("natural");
        drums1->addTag("organic");
        drums1->addTag("roomy");
        drums1->addParameter("kick_tune", "60", "Kick drum tuning");
        drums1->addParameter("snare_crack", "0.7", "Snare crack amount");
        drums1->addParameter("room_mic", "0.4", "Room microphone blend");
        drums1->addParameter("overhead", "0.6", "Overhead microphone level");
        configurations_.push_back(std::move(drums1));
        
        auto drums2 = std::make_unique<ConfigurationEntry>(
            "drums_electronic_kit",
            "Electronic Drum Kit", 
            "electronic_drums",
            "Synthetic electronic drums with punchy samples and effects"
        );
        drums2->addTag("electronic");
        drums2->addTag("synthetic");
        drums2->addTag("punchy");
        drums2->addTag("processed");
        drums2->addParameter("kick_pitch", "40", "Electronic kick pitch");
        drums2->addParameter("snare_snap", "0.8", "Digital snare snap");
        drums2->addParameter("gate", "0.5", "Gate effect amount");
        drums2->addParameter("reverb", "0.3", "Digital reverb");
        configurations_.push_back(std::move(drums2));
        
        // Build index and generate embeddings
        rebuildIndex();
        
        std::cout << "Initialized " << configurations_.size() << " default configurations\n";
    }
    
    /**
     * @brief Rebuild index and generate embeddings
     */
    void rebuildIndex() {
        idToIndex_.clear();
        
        for (size_t i = 0; i < configurations_.size(); ++i) {
            const auto& config = configurations_[i];
            idToIndex_[config->getId()] = i;
            
            // Generate embedding for this configuration
            const_cast<ConfigurationEntry*>(config.get())->generateEmbedding(embeddingEngine_);
        }
        
        std::cout << "Rebuilt index for " << configurations_.size() << " configurations\n";
    }
    
    /**
     * @brief Add configuration entry
     */
    void addConfiguration(std::unique_ptr<ConfigurationEntry> config) {
        config->generateEmbedding(embeddingEngine_);
        ConfigurationId id = config->getId();
        configurations_.push_back(std::move(config));
        idToIndex_[id] = configurations_.size() - 1;
    }
    
    /**
     * @brief Search configurations using FastText embeddings
     */
    std::vector<std::pair<ConfigurationId, float>> search(const std::string& query, 
                                                         size_t maxResults = 10,
                                                         float threshold = 0.0f) {
        lastQuery_ = query;
        lastSearchResults_.clear();
        
        // Generate query embedding
        EmbeddingVector queryEmbedding = embeddingEngine_.getSentenceEmbedding(query);
        
        // Compute similarities
        std::vector<std::pair<size_t, float>> results;
        
        for (size_t i = 0; i < configurations_.size(); ++i) {
            const auto& config = configurations_[i];
            
            // Skip excluded configurations
            if (config->isExcluded()) continue;
            
            float similarity = config->computeSimilarity(queryEmbedding);
            
            if (similarity >= threshold) {
                results.emplace_back(i, similarity);
            }
        }
        
        // Sort by similarity (descending)
        std::sort(results.begin(), results.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Limit results
        if (results.size() > maxResults) {
            results.resize(maxResults);
        }
        
        lastSearchResults_ = results;
        
        // Convert to ConfigurationId format
        std::vector<std::pair<ConfigurationId, float>> finalResults;
        for (const auto& [index, score] : results) {
            finalResults.emplace_back(configurations_[index]->getId(), score);
        }
        
        return finalResults;
    }
    
    /**
     * @brief Search with semantic expansion
     */
    std::vector<std::pair<ConfigurationId, float>> searchWithSemanticExpansion(
        const std::string& query, size_t maxResults = 10, float threshold = 0.0f) {
        
        // Find semantic matches to expand the query
        auto semanticMatches = semanticDb_.findMatches(query, 5, 0.3f);
        
        std::string expandedQuery = query;
        for (const auto& [keyword, score] : semanticMatches) {
            expandedQuery += " " + keyword;
        }
        
        std::cout << "Expanded query: " << expandedQuery << "\n";
        
        return search(expandedQuery, maxResults, threshold);
    }
    
    /**
     * @brief Get configuration by ID
     */
    const ConfigurationEntry* getConfiguration(const ConfigurationId& id) const {
        auto it = idToIndex_.find(id);
        if (it != idToIndex_.end()) {
            return configurations_[it->second].get();
        }
        return nullptr;
    }
    
    /**
     * @brief Boost configuration (increase its ranking)
     */
    bool boostConfiguration(const ConfigurationId& id, float boostAmount = 0.1f) {
        auto it = idToIndex_.find(id);
        if (it != idToIndex_.end()) {
            configurations_[it->second]->setUserBoost(
                configurations_[it->second]->getUserBoost() + boostAmount);
            std::cout << "Boosted " << id << " by " << boostAmount << "\n";
            return true;
        }
        return false;
    }
    
    /**
     * @brief Demote configuration (decrease its ranking)
     */
    bool demoteConfiguration(const ConfigurationId& id, float demoteAmount = 0.1f) {
        auto it = idToIndex_.find(id);
        if (it != idToIndex_.end()) {
            configurations_[it->second]->setUserBoost(
                configurations_[it->second]->getUserBoost() - demoteAmount);
            std::cout << "Demoted " << id << " by " << demoteAmount << "\n";
            return true;
        }
        return false;
    }
    
    /**
     * @brief Exclude configuration from search results
     */
    bool excludeConfiguration(const ConfigurationId& id, bool exclude = true) {
        auto it = idToIndex_.find(id);
        if (it != idToIndex_.end()) {
            configurations_[it->second]->setExcluded(exclude);
            std::cout << (exclude ? "Excluded " : "Included ") << id << "\n";
            return true;
        }
        return false;
    }
    
    /**
     * @brief Select configuration (mark as selected)
     */
    bool selectConfiguration(const ConfigurationId& id) {
        const auto* config = getConfiguration(id);
        if (config) {
            std::cout << "Selected configuration: " << id << "\n";
            config->print();
            return true;
        }
        return false;
    }
    
    /**
     * @brief Print search results
     */
    void printSearchResults(const std::vector<std::pair<ConfigurationId, float>>& results) const {
        if (results.empty()) {
            std::cout << "No results found.\n";
            return;
        }
        
        std::cout << "\nSearch Results (" << results.size() << " found):\n";
        std::cout << std::string(60, '-') << "\n";
        
        for (size_t i = 0; i < results.size(); ++i) {
            const auto& [id, score] = results[i];
            const auto* config = getConfiguration(id);
            
            if (config) {
                std::cout << std::setw(2) << (i + 1) << ". ";
                std::cout << std::setw(20) << std::left << id;
                std::cout << " | Score: " << std::fixed << std::setprecision(3) << std::setw(6) << score;
                std::cout << " | " << config->getName();
                if (config->getUserBoost() != 0.0f) {
                    std::cout << " [Boost: " << std::showpos << config->getUserBoost() << std::noshowpos << "]";
                }
                if (config->isExcluded()) {
                    std::cout << " [EXCLUDED]";
                }
                std::cout << "\n";
                std::cout << "    Type: " << config->getInstrumentType() << "\n";
                std::cout << "    Tags: ";
                const auto& tags = config->getTags();
                for (size_t j = 0; j < tags.size(); ++j) {
                    std::cout << tags[j];
                    if (j < tags.size() - 1) std::cout << ", ";
                }
                std::cout << "\n";
                std::cout << "    Description: " << config->getDescription() << "\n\n";
            }
        }
    }
    
    /**
     * @brief Print system statistics
     */
    void printStatistics() const {
        std::cout << "\nPointing Index System Statistics:\n";
        std::cout << std::string(40, '=') << "\n";
        std::cout << "Total configurations: " << configurations_.size() << "\n";
        
        // Count by instrument type
        std::unordered_map<std::string, int> typeCounts;
        int excludedCount = 0;
        float totalBoost = 0.0f;
        
        for (const auto& config : configurations_) {
            typeCounts[config->getInstrumentType()]++;
            if (config->isExcluded()) excludedCount++;
            totalBoost += config->getUserBoost();
        }
        
        std::cout << "Excluded configurations: " << excludedCount << "\n";
        std::cout << "Average user boost: " << std::fixed << std::setprecision(3) 
                  << (totalBoost / configurations_.size()) << "\n";
        
        std::cout << "\nBy instrument type:\n";
        for (const auto& [type, count] : typeCounts) {
            std::cout << "  " << type << ": " << count << "\n";
        }
        
        if (!lastQuery_.empty()) {
            std::cout << "\nLast search: \"" << lastQuery_ << "\"\n";
            std::cout << "Results found: " << lastSearchResults_.size() << "\n";
        }
        
        // Embedding engine stats
        std::cout << "\nEmbedding Statistics:\n";
        std::cout << "Embedding dimension: " << embeddingEngine_.getDimension() << "\n";
        std::cout << "Vocabulary size: " << embeddingEngine_.getVocabularySize() << "\n";
        
        // Semantic database stats
        semanticDb_.printStatistics();
    }
    
    /**
     * @brief Export dynamic configuration based on current state
     */
    void exportDynamicConfiguration(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file) {
            std::cerr << "Error: Could not open file " << filename << " for writing\n";
            return;
        }
        
        file << "{\n";
        file << "  \"metadata\": {\n";
        file << "    \"export_timestamp\": \"" << std::time(nullptr) << "\",\n";
        file << "    \"total_configurations\": " << configurations_.size() << ",\n";
        file << "    \"embedding_dimension\": " << embeddingEngine_.getDimension() << ",\n";
        file << "    \"last_query\": \"" << lastQuery_ << "\"\n";
        file << "  },\n";
        
        file << "  \"configurations\": [\n";
        
        for (size_t i = 0; i < configurations_.size(); ++i) {
            const auto& config = configurations_[i];
            
            file << "    {\n";
            file << "      \"id\": \"" << config->getId() << "\",\n";
            file << "      \"name\": \"" << config->getName() << "\",\n";
            file << "      \"instrument_type\": \"" << config->getInstrumentType() << "\",\n";
            file << "      \"description\": \"" << config->getDescription() << "\",\n";
            file << "      \"user_boost\": " << config->getUserBoost() << ",\n";
            file << "      \"excluded\": " << (config->isExcluded() ? "true" : "false") << ",\n";
            
            // Tags
            file << "      \"tags\": [";
            const auto& tags = config->getTags();
            for (size_t j = 0; j < tags.size(); ++j) {
                file << "\"" << tags[j] << "\"";
                if (j < tags.size() - 1) file << ", ";
            }
            file << "],\n";
            
            // Parameters
            file << "      \"parameters\": {\n";
            const auto& params = config->getParameters();
            for (size_t j = 0; j < params.size(); ++j) {
                file << "        \"" << params[j].name << "\": {\n";
                file << "          \"value\": \"" << params[j].value << "\",\n";
                file << "          \"description\": \"" << params[j].description << "\"\n";
                file << "        }";
                if (j < params.size() - 1) file << ",";
                file << "\n";
            }
            file << "      }\n";
            
            file << "    }";
            if (i < configurations_.size() - 1) file << ",";
            file << "\n";
        }
        
        file << "  ]\n";
        file << "}\n";
        
        file.close();
        std::cout << "Exported dynamic configuration to " << filename << "\n";
    }
    
    /**
     * @brief Get all configuration IDs
     */
    std::vector<ConfigurationId> getAllConfigurationIds() const {
        std::vector<ConfigurationId> ids;
        for (const auto& config : configurations_) {
            ids.push_back(config->getId());
        }
        return ids;
    }
    
    /**
     * @brief Get configuration count
     */
    size_t getConfigurationCount() const {
        return configurations_.size();
    }
};

// ============================================================================
// COMMAND LINE INTERFACE
// ============================================================================

/**
 * @brief Interactive command line interface
 */
class CommandLineInterface {
private:
    FastTextEmbeddingEngine embeddingEngine_;
    EnhancedSemanticDatabase semanticDb_;
    PointingIndexSystem indexSystem_;
    bool running_;
    
    /**
     * @brief Print help message
     */
    void printHelp() const {
        std::cout << "\nAvailable Commands:\n";
        std::cout << std::string(50, '=') << "\n";
        std::cout << "search <query>              - Search configurations\n";
        std::cout << "search_semantic <query>     - Search with semantic expansion\n";
        std::cout << "select <config_id>          - Select and display configuration\n";
        std::cout << "boost <config_id> [amount]  - Boost configuration ranking\n";
        std::cout << "demote <config_id> [amount] - Demote configuration ranking\n";
        std::cout << "exclude <config_id>         - Exclude configuration from search\n";
        std::cout << "include <config_id>         - Include configuration in search\n";
        std::cout << "stats                       - Print system statistics\n";
        std::cout << "export <filename>           - Export dynamic configuration\n";
        std::cout << "list                        - List all configuration IDs\n";
        std::cout << "similar <word>              - Find similar words\n";
        std::cout << "semantic <query>            - Find semantic matches\n";
        std::cout << "help                        - Show this help message\n";
        std::cout << "quit                        - Exit the program\n";
        std::cout << std::string(50, '=') << "\n";
    }
    
    /**
     * @brief Parse and execute command
     */
    void executeCommand(const std::string& input) {
        std::istringstream iss(input);
        std::string command;
        iss >> command;
        
                 if (command == "search") {
             std::string query;
             std::getline(iss, query);
             if (!query.empty() && query[0] == ' ') {
                 query = query.length() > 1 ? query.substr(1) : "";
             }
             
             if (query.empty()) {
                 std::cout << "Usage: search <query>\n";
                 return;
             }
            
            auto results = indexSystem_.search(query, 10, 0.1f);
            indexSystem_.printSearchResults(results);
            
                 } else if (command == "search_semantic") {
             std::string query;
             std::getline(iss, query);
             if (!query.empty() && query[0] == ' ') {
                 query = query.length() > 1 ? query.substr(1) : "";
             }
             
             if (query.empty()) {
                 std::cout << "Usage: search_semantic <query>\n";
                 return;
             }
            
            auto results = indexSystem_.searchWithSemanticExpansion(query, 10, 0.1f);
            indexSystem_.printSearchResults(results);
            
        } else if (command == "select") {
            std::string configId;
            iss >> configId;
            
            if (configId.empty()) {
                std::cout << "Usage: select <config_id>\n";
                return;
            }
            
            if (!indexSystem_.selectConfiguration(configId)) {
                std::cout << "Configuration not found: " << configId << "\n";
            }
            
        } else if (command == "boost") {
            std::string configId;
            float amount = 0.1f;
            iss >> configId >> amount;
            
            if (configId.empty()) {
                std::cout << "Usage: boost <config_id> [amount]\n";
                return;
            }
            
            if (!indexSystem_.boostConfiguration(configId, amount)) {
                std::cout << "Configuration not found: " << configId << "\n";
            }
            
        } else if (command == "demote") {
            std::string configId;
            float amount = 0.1f;
            iss >> configId >> amount;
            
            if (configId.empty()) {
                std::cout << "Usage: demote <config_id> [amount]\n";
                return;
            }
            
            if (!indexSystem_.demoteConfiguration(configId, amount)) {
                std::cout << "Configuration not found: " << configId << "\n";
            }
            
        } else if (command == "exclude") {
            std::string configId;
            iss >> configId;
            
            if (configId.empty()) {
                std::cout << "Usage: exclude <config_id>\n";
                return;
            }
            
            if (!indexSystem_.excludeConfiguration(configId, true)) {
                std::cout << "Configuration not found: " << configId << "\n";
            }
            
        } else if (command == "include") {
            std::string configId;
            iss >> configId;
            
            if (configId.empty()) {
                std::cout << "Usage: include <config_id>\n";
                return;
            }
            
            if (!indexSystem_.excludeConfiguration(configId, false)) {
                std::cout << "Configuration not found: " << configId << "\n";
            }
            
        } else if (command == "stats") {
            indexSystem_.printStatistics();
            
        } else if (command == "export") {
            std::string filename;
            iss >> filename;
            
            if (filename.empty()) {
                filename = "dynamic_config.json";
            }
            
            indexSystem_.exportDynamicConfiguration(filename);
            
        } else if (command == "list") {
            auto ids = indexSystem_.getAllConfigurationIds();
            std::cout << "\nAll Configuration IDs (" << ids.size() << "):\n";
            std::cout << std::string(30, '-') << "\n";
            for (const auto& id : ids) {
                const auto* config = indexSystem_.getConfiguration(id);
                std::cout << std::setw(25) << std::left << id;
                if (config) {
                    std::cout << " - " << config->getName();
                    if (config->isExcluded()) std::cout << " [EXCLUDED]";
                    if (config->getUserBoost() != 0.0f) {
                        std::cout << " [Boost: " << std::showpos << config->getUserBoost() << std::noshowpos << "]";
                    }
                }
                std::cout << "\n";
            }
            
        } else if (command == "similar") {
            std::string word;
            iss >> word;
            
            if (word.empty()) {
                std::cout << "Usage: similar <word>\n";
                return;
            }
            
            auto similar = embeddingEngine_.findSimilarWords(word, 10);
            std::cout << "\nWords similar to '" << word << "':\n";
            std::cout << std::string(30, '-') << "\n";
            for (const auto& [w, score] : similar) {
                std::cout << std::setw(20) << std::left << w 
                          << " | " << std::fixed << std::setprecision(3) << score << "\n";
            }
            
                 } else if (command == "semantic") {
             std::string query;
             std::getline(iss, query);
             if (!query.empty() && query[0] == ' ') {
                 query = query.length() > 1 ? query.substr(1) : "";
             }
             
             if (query.empty()) {
                 std::cout << "Usage: semantic <query>\n";
                 return;
             }
            
            auto matches = semanticDb_.findMatches(query, 10, 0.1f);
            std::cout << "\nSemantic matches for '" << query << "':\n";
            std::cout << std::string(30, '-') << "\n";
            for (const auto& [keyword, score] : matches) {
                const auto* entry = semanticDb_.getEntry(keyword);
                std::cout << std::setw(15) << std::left << keyword 
                          << " | " << std::fixed << std::setprecision(3) << score;
                if (entry) {
                    std::cout << " | " << entry->category;
                }
                std::cout << "\n";
            }
            
        } else if (command == "help") {
            printHelp();
            
        } else if (command == "quit" || command == "exit") {
            running_ = false;
            std::cout << "Goodbye!\n";
            
        } else if (!command.empty()) {
            std::cout << "Unknown command: " << command << "\n";
            std::cout << "Type 'help' for available commands.\n";
        }
    }
    
public:
    /**
     * @brief Constructor
     */
    CommandLineInterface() 
        : embeddingEngine_(100, 3, 6)
        , semanticDb_(embeddingEngine_)
        , indexSystem_(embeddingEngine_, semanticDb_)
        , running_(true) {
        
        std::cout << "AI Synthesis System v2.0\n";
        std::cout << "Integrated FastText Embedding Engine with Pointing Index\n";
        std::cout << std::string(60, '=') << "\n";
        
        // Initialize system
        std::cout << "Initializing system...\n";
        
        std::cout << "System ready!\n";
        std::cout << "Type 'help' for available commands.\n\n";
    }
    
    /**
     * @brief Run interactive loop
     */
    void run() {
        std::string input;
        
        while (running_) {
            std::cout << "ai_synthesis> ";
            std::getline(std::cin, input);
            
            if (!input.empty()) {
                executeCommand(input);
                std::cout << "\n";
            }
        }
    }
};

} // namespace AISynthesis

// ============================================================================
// MAIN FUNCTION
// ============================================================================

int main() {
    try {
        AISynthesis::CommandLineInterface cli;
        cli.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred\n";
        return 1;
    }
}