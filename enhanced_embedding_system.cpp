#include "json.hpp"
#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <regex>
#include <cmath>
#include <random>
#include <numeric>
#include <set>
#include <iomanip>

using namespace std;
using json = nlohmann::json;

// Enhanced embedding system with more sophisticated text processing
class FastTextEmbeddingEngine {
private:
    unordered_map<string, vector<float>> wordEmbeddings;
    unordered_map<string, vector<float>> subwordEmbeddings;
    unordered_map<string, vector<float>> cachedSentenceEmbeddings;
    
    const int EMBEDDING_DIM = 100;
    const int MIN_WORD_LENGTH = 3;
    const int MAX_NGRAM = 6;
    
    mt19937 rng;
    normal_distribution<float> normalDist;
    
public:
    FastTextEmbeddingEngine() : rng(42), normalDist(0.0f, 0.1f) {
        loadEnhancedEmbeddings();
    }
    
private:
    void loadEnhancedEmbeddings() {
        cout << "Loading enhanced FastText-style embeddings..." << endl;
        
        // Music and audio domain vocabulary with synthetic embeddings
        vector<string> musicVocab = {
            // Timbral qualities
            "warm", "bright", "dark", "smooth", "rough", "sharp", "soft", "hard",
            "thick", "thin", "rich", "sparse", "dense", "clear", "muddy", "crisp",
            "mellow", "harsh", "sweet", "bitter", "round", "angular", "organic", "synthetic",
            
            // Emotional qualities  
            "aggressive", "calm", "peaceful", "energetic", "dreamy", "mysterious",
            "intimate", "bold", "delicate", "powerful", "gentle", "fierce", "serene",
            "chaotic", "stable", "unstable", "flowing", "choppy", "smooth", "jagged",
            
            // Technical terms
            "attack", "decay", "sustain", "release", "envelope", "filter", "resonance",
            "cutoff", "frequency", "amplitude", "oscillator", "modulation", "vibrato",
            "tremolo", "chorus", "reverb", "delay", "echo", "compression", "distortion",
            
            // Instruments
            "guitar", "bass", "piano", "drums", "violin", "saxophone", "trumpet",
            "flute", "synthesizer", "keyboard", "vocal", "strings", "brass", "woodwind",
            
            // Genres/Styles
            "classical", "jazz", "rock", "electronic", "ambient", "folk", "blues",
            "metal", "pop", "country", "funk", "soul", "techno", "house", "dubstep",
            
            // Materials/Physical
            "wood", "metal", "plastic", "glass", "air", "water", "stone", "silk",
            "rubber", "ceramic", "organic", "digital", "analog", "virtual", "physical"
        };
        
        // Generate contextually meaningful embeddings
        generateContextualEmbeddings(musicVocab);
        
        // Generate subword embeddings for OOV handling
        generateSubwordEmbeddings(musicVocab);
        
        cout << "Generated " << wordEmbeddings.size() << " word embeddings and " 
             << subwordEmbeddings.size() << " subword embeddings." << endl;
    }
    
    void generateContextualEmbeddings(const vector<string>& vocab) {
        // Create semantic clusters and generate embeddings within clusters
        map<string, vector<string>> semanticClusters = {
            {"timbral_warm", {"warm", "soft", "mellow", "smooth", "round", "organic", "sweet"}},
            {"timbral_bright", {"bright", "sharp", "crisp", "clear", "harsh", "thin", "metallic"}},
            {"timbral_dark", {"dark", "thick", "dense", "deep", "rich", "heavy", "woody"}},
            {"emotional_calm", {"calm", "peaceful", "serene", "gentle", "flowing", "dreamy"}},
            {"emotional_energetic", {"aggressive", "energetic", "bold", "powerful", "fierce", "driving"}},
            {"technical_envelope", {"attack", "decay", "sustain", "release", "envelope", "dynamics"}},
            {"technical_filter", {"filter", "cutoff", "resonance", "frequency", "sweep", "modulation"}},
            {"instruments_string", {"guitar", "bass", "violin", "strings", "plucked", "bowed"}},
            {"instruments_electronic", {"synthesizer", "digital", "virtual", "electronic", "processed"}},
            {"effects_spatial", {"reverb", "delay", "echo", "space", "depth", "ambience"}},
            {"effects_modulation", {"chorus", "vibrato", "tremolo", "phaser", "flanger", "modulation"}}
        };
        
        // Generate cluster centers
        map<string, vector<float>> clusterCenters;
        for (const auto& [clusterName, words] : semanticClusters) {
            clusterCenters[clusterName] = generateRandomVector(EMBEDDING_DIM);
        }
        
        // Generate word embeddings around cluster centers
        for (const auto& [clusterName, words] : semanticClusters) {
            const auto& center = clusterCenters[clusterName];
            
            for (const string& word : words) {
                if (find(vocab.begin(), vocab.end(), word) != vocab.end()) {
                    wordEmbeddings[word] = perturbVector(center, 0.3f);
                }
            }
        }
        
        // Generate embeddings for remaining vocabulary
        for (const string& word : vocab) {
            if (wordEmbeddings.find(word) == wordEmbeddings.end()) {
                wordEmbeddings[word] = generateRandomVector(EMBEDDING_DIM);
            }
        }
        
        // Create semantic relationships between related terms
        createSemanticRelationships();
    }
    
    void createSemanticRelationships() {
        // Define semantic relationships and adjust embeddings accordingly
        vector<pair<string, string>> synonymPairs = {
            {"warm", "soft"}, {"bright", "sharp"}, {"calm", "peaceful"},
            {"aggressive", "fierce"}, {"attack", "onset"}, {"decay", "release"},
            {"reverb", "echo"}, {"guitar", "strings"}, {"bass", "low"},
            {"synthesizer", "electronic"}, {"organic", "natural"}, {"smooth", "flowing"}
        };
        
        vector<pair<string, string>> oppositePairs = {
            {"warm", "bright"}, {"soft", "harsh"}, {"calm", "aggressive"},
            {"thick", "thin"}, {"dark", "bright"}, {"smooth", "rough"},
            {"organic", "synthetic"}, {"gentle", "fierce"}, {"mellow", "sharp"}
        };
        
        // Adjust embeddings to reflect synonym relationships (similar)
        for (const auto& [word1, word2] : synonymPairs) {
            if (wordEmbeddings.find(word1) != wordEmbeddings.end() &&
                wordEmbeddings.find(word2) != wordEmbeddings.end()) {
                
                auto& emb1 = wordEmbeddings[word1];
                auto& emb2 = wordEmbeddings[word2];
                
                // Move embeddings closer together
                for (int i = 0; i < EMBEDDING_DIM; ++i) {
                    float avg = (emb1[i] + emb2[i]) / 2.0f;
                    emb1[i] = 0.8f * emb1[i] + 0.2f * avg;
                    emb2[i] = 0.8f * emb2[i] + 0.2f * avg;
                }
            }
        }
        
        // Adjust embeddings to reflect opposite relationships (dissimilar)
        for (const auto& [word1, word2] : oppositePairs) {
            if (wordEmbeddings.find(word1) != wordEmbeddings.end() &&
                wordEmbeddings.find(word2) != wordEmbeddings.end()) {
                
                auto& emb1 = wordEmbeddings[word1];
                auto& emb2 = wordEmbeddings[word2];
                
                // Push embeddings apart
                for (int i = 0; i < EMBEDDING_DIM; ++i) {
                    float diff = emb1[i] - emb2[i];
                    emb1[i] += 0.1f * diff;
                    emb2[i] -= 0.1f * diff;
                }
            }
        }
    }
    
    void generateSubwordEmbeddings(const vector<string>& vocab) {
        set<string> ngrams;
        
        for (const string& word : vocab) {
            auto wordNgrams = extractNgrams(word);
            ngrams.insert(wordNgrams.begin(), wordNgrams.end());
        }
        
        for (const string& ngram : ngrams) {
            subwordEmbeddings[ngram] = generateRandomVector(EMBEDDING_DIM);
        }
    }
    
    vector<string> extractNgrams(const string& word) {
        vector<string> ngrams;
        string paddedWord = "<" + word + ">";
        
        for (int n = MIN_WORD_LENGTH; n <= min(MAX_NGRAM, (int)paddedWord.length()); ++n) {
            for (int i = 0; i <= (int)paddedWord.length() - n; ++i) {
                ngrams.push_back(paddedWord.substr(i, n));
            }
        }
        
        return ngrams;
    }
    
    vector<float> generateRandomVector(int dim) {
        vector<float> vec(dim);
        for (int i = 0; i < dim; ++i) {
            vec[i] = normalDist(rng);
        }
        
        // Normalize
        float norm = sqrt(inner_product(vec.begin(), vec.end(), vec.begin(), 0.0f));
        if (norm > 0) {
            for (float& val : vec) {
                val /= norm;
            }
        }
        
        return vec;
    }
    
    vector<float> perturbVector(const vector<float>& base, float variance) {
        vector<float> result(base.size());
        normal_distribution<float> perturbDist(0.0f, variance);
        
        for (size_t i = 0; i < base.size(); ++i) {
            result[i] = base[i] + perturbDist(rng);
        }
        
        // Normalize
        float norm = sqrt(inner_product(result.begin(), result.end(), result.begin(), 0.0f));
        if (norm > 0) {
            for (float& val : result) {
                val /= norm;
            }
        }
        
        return result;
    }

public:
    vector<float> getWordEmbedding(const string& word) {
        string cleanWord = cleanText(word);
        
        // Direct lookup
        if (wordEmbeddings.find(cleanWord) != wordEmbeddings.end()) {
            return wordEmbeddings[cleanWord];
        }
        
        // Subword fallback
        return getSubwordEmbedding(cleanWord);
    }
    
    vector<float> getSubwordEmbedding(const string& word) {
        auto ngrams = extractNgrams(word);
        vector<float> result(EMBEDDING_DIM, 0.0f);
        int count = 0;
        
        for (const string& ngram : ngrams) {
            if (subwordEmbeddings.find(ngram) != subwordEmbeddings.end()) {
                const auto& ngramEmb = subwordEmbeddings[ngram];
                for (int i = 0; i < EMBEDDING_DIM; ++i) {
                    result[i] += ngramEmb[i];
                }
                count++;
            }
        }
        
        if (count > 0) {
            for (float& val : result) {
                val /= count;
            }
        } else {
            // Fallback to random embedding
            result = generateRandomVector(EMBEDDING_DIM);
        }
        
        return result;
    }
    
    vector<float> getSentenceEmbedding(const string& text) {
        string cacheKey = cleanText(text);
        
        if (cachedSentenceEmbeddings.find(cacheKey) != cachedSentenceEmbeddings.end()) {
            return cachedSentenceEmbeddings[cacheKey];
        }
        
        vector<string> words = tokenize(text);
        vector<float> result(EMBEDDING_DIM, 0.0f);
        int count = 0;
        
        for (const string& word : words) {
            if (word.length() >= 2) { // Filter very short words
                auto wordEmb = getWordEmbedding(word);
                for (int i = 0; i < EMBEDDING_DIM; ++i) {
                    result[i] += wordEmb[i];
                }
                count++;
            }
        }
        
        if (count > 0) {
            for (float& val : result) {
                val /= count;
            }
        }
        
        // Apply TF-IDF-like weighting for important terms
        result = applyTermWeighting(result, words);
        
        cachedSentenceEmbeddings[cacheKey] = result;
        return result;
    }
    
    float computeCosineSimilarity(const vector<float>& a, const vector<float>& b) {
        if (a.size() != b.size()) return 0.0f;
        
        float dotProduct = inner_product(a.begin(), a.end(), b.begin(), 0.0f);
        float normA = sqrt(inner_product(a.begin(), a.end(), a.begin(), 0.0f));
        float normB = sqrt(inner_product(b.begin(), b.end(), b.begin(), 0.0f));
        
        if (normA == 0.0f || normB == 0.0f) return 0.0f;
        
        return dotProduct / (normA * normB);
    }
    
    vector<pair<string, float>> findSimilarWords(const string& word, int topK = 5) {
        auto queryEmb = getWordEmbedding(word);
        vector<pair<string, float>> similarities;
        
        for (const auto& [candWord, candEmb] : wordEmbeddings) {
            if (candWord != word) {
                float sim = computeCosineSimilarity(queryEmb, candEmb);
                similarities.emplace_back(candWord, sim);
            }
        }
        
        sort(similarities.begin(), similarities.end(),
             [](const auto& a, const auto& b) { return a.second > b.second; });
        
        if (similarities.size() > topK) {
            similarities.resize(topK);
        }
        
        return similarities;
    }

private:
    string cleanText(const string& text) {
        string result = text;
        transform(result.begin(), result.end(), result.begin(), ::tolower);
        result = regex_replace(result, regex("[^a-zA-Z0-9\\s]"), " ");
        result = regex_replace(result, regex("\\s+"), " ");
        
        // Trim
        size_t start = result.find_first_not_of(" ");
        if (start != string::npos) {
            size_t end = result.find_last_not_of(" ");
            result = result.substr(start, end - start + 1);
        }
        
        return result;
    }
    
    vector<string> tokenize(const string& text) {
        vector<string> tokens;
        stringstream ss(cleanText(text));
        string token;
        
        while (ss >> token) {
            if (!token.empty()) {
                tokens.push_back(token);
            }
        }
        
        return tokens;
    }
    
    vector<float> applyTermWeighting(const vector<float>& baseEmbedding, const vector<string>& words) {
        // Simple term weighting based on music domain importance
        set<string> importantTerms = {
            "warm", "bright", "aggressive", "calm", "attack", "decay", "sustain", "release",
            "reverb", "delay", "guitar", "bass", "synthesizer", "filter", "resonance"
        };
        
        vector<float> result = baseEmbedding;
        float boost = 1.0f;
        
        for (const string& word : words) {
            if (importantTerms.find(word) != importantTerms.end()) {
                boost += 0.2f;
            }
        }
        
        for (float& val : result) {
            val *= boost;
        }
        
        return result;
    }

public:
    void printStatistics() {
        cout << "\n=== EMBEDDING ENGINE STATISTICS ===" << endl;
        cout << "Word embeddings: " << wordEmbeddings.size() << endl;
        cout << "Subword embeddings: " << subwordEmbeddings.size() << endl;
        cout << "Cached sentence embeddings: " << cachedSentenceEmbeddings.size() << endl;
        cout << "Embedding dimension: " << EMBEDDING_DIM << endl;
        
        // Show some similarity examples
        cout << "\nSimilarity examples:" << endl;
        vector<string> testWords = {"warm", "bright", "aggressive", "attack", "guitar"};
        
        for (const string& word : testWords) {
            auto similar = findSimilarWords(word, 3);
            cout << "  " << word << ": ";
            for (size_t i = 0; i < similar.size(); ++i) {
                if (i > 0) cout << ", ";
                cout << similar[i].first << "(" << fixed << setprecision(2) << similar[i].second << ")";
            }
            cout << endl;
        }
        
        cout << "====================================" << endl;
    }
};

// Enhanced SKD with richer semantic information
class EnhancedSemanticDatabase {
private:
    json semanticData;
    FastTextEmbeddingEngine* embeddingEngine;
    
public:
    EnhancedSemanticDatabase(FastTextEmbeddingEngine* engine) : embeddingEngine(engine) {
        buildEnhancedDatabase();
    }
    
private:
    void buildEnhancedDatabase() {
        cout << "Building enhanced semantic database..." << endl;
        
        semanticData = json::object();
        
        // Define comprehensive semantic entries
        vector<tuple<string, string, vector<string>, string, vector<string>, vector<string>>> entries = {
            // Term, Category, Aliases, Explanation, Context, Related
            {"warm", "timbral", {"soft", "mellow", "cozy", "smooth"}, 
             "Produces soft, comfortable tones with rounded harmonics and gentle character",
             {"acoustic", "classical", "jazz", "intimate"},
             {"guitar", "piano", "strings", "woodwind"}},
             
            {"bright", "timbral", {"sharp", "crisp", "clear", "cutting"},
             "Creates clear, penetrating tones with enhanced high frequencies and presence",
             {"electric", "pop", "rock", "lead"},
             {"synthesizer", "electric_guitar", "brass", "percussion"}},
             
            {"aggressive", "emotional", {"fierce", "intense", "driving", "powerful"},
             "Delivers assertive, forceful sounds with strong attack and commanding presence",
             {"rock", "metal", "electronic", "energetic"},
             {"distortion", "compression", "fast_attack", "high_resonance"}},
             
            {"calm", "emotional", {"peaceful", "serene", "gentle", "tranquil"},
             "Produces soothing, relaxed sounds with gentle dynamics and flowing character",
             {"ambient", "new_age", "meditation", "background"},
             {"reverb", "slow_attack", "low_resonance", "sustained"}},
             
            {"attack", "parameter", {"onset", "start", "initial", "trigger"},
             "Controls the speed at which a sound reaches full volume when triggered",
             {"envelope", "dynamics", "articulation", "timing"},
             {"decay", "sustain", "release", "envelope_shape"}},
             
            {"reverb", "effect", {"echo", "ambience", "space", "hall"},
             "Adds spatial depth and realistic acoustic environment simulation",
             {"spatial", "depth", "realism", "atmosphere"},
             {"delay", "chorus", "room_size", "decay_time"}},
             
            {"filter", "processing", {"eq", "frequency", "tone", "timbre"},
             "Shapes the frequency content by attenuating or emphasizing certain ranges",
             {"tone_shaping", "frequency_control", "timbre_modification"},
             {"cutoff", "resonance", "slope", "envelope_amount"}},
             
            {"guitar", "instrument", {"strings", "fretted", "plucked", "acoustic_electric"},
             "Versatile stringed instrument capable of diverse tonal expressions",
             {"popular_music", "classical", "folk", "electric"},
             {"bass", "mandolin", "banjo", "ukulele"}},
             
            {"synthesizer", "instrument", {"synth", "electronic", "digital", "virtual"},
             "Electronic instrument capable of generating and manipulating synthetic sounds",
             {"electronic", "experimental", "pop", "dance"},
             {"oscillator", "filter", "envelope", "modulation"}}
        };
        
        for (const auto& [term, category, aliases, explanation, context, related] : entries) {
            json entry = json::object();
            entry["category"] = category;
            entry["aliases"] = aliases;
            entry["explanation"] = explanation;
            entry["context"] = context;
            entry["related"] = related;
            entry["score"] = 1.0f;
            
            // Generate embedding for the term and its description
            string embeddingText = term + " " + explanation;
            for (const string& alias : aliases) {
                embeddingText += " " + alias;
            }
            
            entry["embedding"] = embeddingEngine->getSentenceEmbedding(embeddingText);
            
            // Compute relationships to other terms
            entry["relationships"] = computeRelationships(term, entry);
            
            semanticData[term] = entry;
        }
        
        cout << "Built enhanced semantic database with " << semanticData.size() << " entries." << endl;
    }
    
    json computeRelationships(const string& term, const json& entry) {
        json relationships = json::object();
        
        if (!entry.contains("embedding")) return relationships;
        
        vector<float> termEmbedding = entry["embedding"];
        
        for (const auto& [otherTerm, otherEntry] : semanticData.items()) {
            if (otherTerm == term) continue;
            
            if (otherEntry.contains("embedding")) {
                vector<float> otherEmbedding = otherEntry["embedding"];
                float similarity = embeddingEngine->computeCosineSimilarity(termEmbedding, otherEmbedding);
                
                if (similarity > 0.5f) {
                    relationships[otherTerm] = similarity;
                }
            }
        }
        
        return relationships;
    }

public:
    json getSemanticEntry(const string& term) {
        string lowerTerm = term;
        transform(lowerTerm.begin(), lowerTerm.end(), lowerTerm.begin(), ::tolower);
        
        if (semanticData.contains(lowerTerm)) {
            return semanticData[lowerTerm];
        }
        
        // Try alias matching
        for (const auto& [key, entry] : semanticData.items()) {
            if (entry.contains("aliases")) {
                for (const auto& alias : entry["aliases"]) {
                    if (alias.get<string>() == lowerTerm) {
                        return entry;
                    }
                }
            }
        }
        
        return json::object();
    }
    
    vector<string> findSemanticallySimilar(const string& term, float threshold = 0.7f) {
        vector<string> similar;
        auto termEntry = getSemanticEntry(term);
        
        if (termEntry.empty() || !termEntry.contains("embedding")) {
            return similar;
        }
        
        vector<float> termEmbedding = termEntry["embedding"];
        
        for (const auto& [candidateTerm, candidateEntry] : semanticData.items()) {
            if (candidateTerm == term) continue;
            
            if (candidateEntry.contains("embedding")) {
                vector<float> candidateEmbedding = candidateEntry["embedding"];
                float similarity = embeddingEngine->computeCosineSimilarity(termEmbedding, candidateEmbedding);
                
                if (similarity >= threshold) {
                    similar.push_back(candidateTerm);
                }
            }
        }
        
        return similar;
    }
    
    string getExplanation(const string& term) {
        auto entry = getSemanticEntry(term);
        if (!entry.empty() && entry.contains("explanation")) {
            return entry["explanation"].get<string>();
        }
        return "No explanation available for '" + term + "'";
    }
    
    vector<string> getRelatedTerms(const string& term) {
        auto entry = getSemanticEntry(term);
        vector<string> related;
        
        if (!entry.empty() && entry.contains("related")) {
            for (const auto& relatedTerm : entry["related"]) {
                related.push_back(relatedTerm.get<string>());
            }
        }
        
        return related;
    }
    
    void printDatabaseStatistics() {
        cout << "\n=== SEMANTIC DATABASE STATISTICS ===" << endl;
        cout << "Total semantic entries: " << semanticData.size() << endl;
        
        map<string, int> categoryCount;
        for (const auto& [term, entry] : semanticData.items()) {
            if (entry.contains("category")) {
                categoryCount[entry["category"].get<string>()]++;
            }
        }
        
        cout << "Entries by category:" << endl;
        for (const auto& [category, count] : categoryCount) {
            cout << "  " << category << ": " << count << endl;
        }
        
        cout << "====================================" << endl;
    }
};

// Test and demo functions
void runEmbeddingTests() {
    cout << "\n=== RUNNING EMBEDDING TESTS ===" << endl;
    
    FastTextEmbeddingEngine engine;
    EnhancedSemanticDatabase semanticDb(&engine);
    
    // Test word similarities
    vector<string> testWords = {"warm", "bright", "aggressive", "guitar", "reverb"};
    
    cout << "\nWord similarity tests:" << endl;
    for (const string& word : testWords) {
        auto similarities = engine.findSimilarWords(word, 3);
        cout << word << " -> ";
        for (size_t i = 0; i < similarities.size(); ++i) {
            if (i > 0) cout << ", ";
            cout << similarities[i].first << "(" << similarities[i].second << ")";
        }
        cout << endl;
    }
    
    // Test sentence embeddings
    cout << "\nSentence similarity tests:" << endl;
    vector<string> sentences = {
        "warm acoustic guitar with soft reverb",
        "bright electric lead with aggressive distortion", 
        "calm ambient pad with gentle attack",
        "punchy bass with quick decay"
    };
    
    for (size_t i = 0; i < sentences.size(); ++i) {
        for (size_t j = i + 1; j < sentences.size(); ++j) {
            auto emb1 = engine.getSentenceEmbedding(sentences[i]);
            auto emb2 = engine.getSentenceEmbedding(sentences[j]);
            float sim = engine.computeCosineSimilarity(emb1, emb2);
            
            cout << "\"" << sentences[i] << "\" <-> \"" << sentences[j] << "\": " 
                 << fixed << setprecision(3) << sim << endl;
        }
    }
    
    // Test semantic database
    cout << "\nSemantic database tests:" << endl;
    for (const string& term : {"warm", "bright", "attack", "guitar"}) {
        cout << term << ":" << endl;
        cout << "  Explanation: " << semanticDb.getExplanation(term) << endl;
        
        auto related = semanticDb.getRelatedTerms(term);
        cout << "  Related: ";
        for (size_t i = 0; i < related.size(); ++i) {
            if (i > 0) cout << ", ";
            cout << related[i];
        }
        cout << endl;
        
        auto similar = semanticDb.findSemanticallySimilar(term, 0.6f);
        cout << "  Similar: ";
        for (size_t i = 0; i < similar.size() && i < 3; ++i) {
            if (i > 0) cout << ", ";
            cout << similar[i];
        }
        cout << endl << endl;
    }
    
    // Print statistics
    engine.printStatistics();
    semanticDb.printDatabaseStatistics();
    
    cout << "=== EMBEDDING TESTS COMPLETE ===" << endl;
}

int main() {
    cout << "Enhanced Embedding System - FastText-style Semantic Processing" << endl;
    cout << "=============================================================" << endl;
    
    try {
        runEmbeddingTests();
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}