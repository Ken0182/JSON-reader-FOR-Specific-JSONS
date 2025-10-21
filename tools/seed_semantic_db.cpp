#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <algorithm>
#include <cmath>

#include "json.hpp"
#include "text_utils.hpp"
#include "semantic_db.hpp"
#include "sentence_encoder.hpp"

// Seeder for semantic.db: extracts tags from repository JSON/MD files,
// encodes them with SentenceEncoder, and stores embeddings/IDF/config in SQLite.

namespace fs = std::filesystem;
using json = nlohmann::json;
using audio_config::SemanticDatabase;
using audio_config::SentenceEncoder;
using audio_config::TextUtils;

struct Options {
    std::string outDbPath = "data/semantic.db";
    int dimension = 100;
    std::vector<std::string> jsonPaths;         // Additional JSON sources
    bool includeMarkdown = true;                // Scan *.md files in repo
};

static void printUsage(const char* prog) {
    std::cout << "Seed semantic.db with default knowledge\n\n";
    std::cout << "Usage: " << prog << " [--out data/semantic.db] [--dim 100] [--json <file> ...] [--no-md]\n\n";
    std::cout << "Sources: data/clean_config.json (if present), group.json, guitar.json, structure.json, *.md\n";
}

static bool readFileText(const fs::path& p, std::string& out) {
    std::ifstream f(p);
    if (!f.is_open()) return false;
    std::ostringstream ss; ss << f.rdbuf();
    out = ss.str();
    return true;
}

static void addToken(std::set<std::string>& tags, const std::string& token) {
    if (token.empty()) return;
    // Basic heuristics: keep alphabetic tokens, length 3-24
    if (token.size() < 3 || token.size() > 24) return;
    if (!std::all_of(token.begin(), token.end(), [](unsigned char c){ return std::isalpha(c) || c=='-'; })) return;
    static const std::unordered_set<std::string> stop = {
        "the","and","with","that","this","into","from","using","audio","system","config","json",
        "vector","vectors","score","scoring","search","token","tokens","plugin","format","sample",
        "range","host","vst","vst3","au","aax","clap","linux","windows","macos","build","make",
        "data","clean","weights","layer","layers","semantic","knowledge","database","sqlite","index",
        "query","queries","signal","signals","track","tracking","technical","musical","role","layering",
        "compatibility","analysis","engine","encoder","hash","onnx","model","models","class","struct"
    };
    if (stop.count(token)) return;
    tags.insert(token);
}

static void extractTokensFromMarkdown(const fs::path& root, std::set<std::string>& tags) {
    for (auto& it : fs::recursive_directory_iterator(root)) {
        if (!it.is_regular_file()) continue;
        if (it.path().extension() != ".md") continue;
        std::string content;
        if (!readFileText(it.path(), content)) continue;
        auto toks = TextUtils::tokenize(content);
        for (const auto& t : toks) addToken(tags, t);
    }
}

// Extract domain tags from a JSON object by looking at common fields
static void extractTagsFromJsonObject(const json& obj, std::set<std::string>& tags) {
    auto tryAddString = [&](const std::string& v) {
        auto toks = TextUtils::tokenize(v);
        for (const auto& t : toks) addToken(tags, t);
    };

    // Common semantic fields
    auto addIfString = [&](const json& j, const char* key){
        if (j.contains(key) && j[key].is_string()) tryAddString(j[key].get<std::string>());
    };

    addIfString(obj, "timbral");
    addIfString(obj, "dynamic");
    addIfString(obj, "material");

    // emotional: [{"tag": "warm", "weight": 0.8}]
    if (obj.contains("emotional") && obj["emotional"].is_array()) {
        for (const auto& e : obj["emotional"]) {
            if (e.is_object() && e.contains("tag") && e["tag"].is_string()) {
                tryAddString(e["tag"].get<std::string>());
            }
        }
    }
}

static void extractTokensFromJsonFile(const fs::path& p, std::set<std::string>& tags) {
    try {
        std::ifstream f(p);
        if (!f.is_open()) return;
        json j; f >> j;

        // Heuristic traversal: examine all objects in JSON tree
        std::vector<const json*> stack{ &j };
        while (!stack.empty()) {
            const json* cur = stack.back();
            stack.pop_back();
            if (cur->is_object()) {
                // Look for common nested fields that hold descriptors
                if (cur->contains("soundCharacteristics") && (*cur)["soundCharacteristics"].is_object()) {
                    extractTagsFromJsonObject((*cur)["soundCharacteristics"], tags);
                }
                if (cur->contains("sound_characteristics") && (*cur)["sound_characteristics"].is_object()) {
                    extractTagsFromJsonObject((*cur)["sound_characteristics"], tags);
                }
                // Push children
                for (auto it = cur->begin(); it != cur->end(); ++it) {
                    if (it->is_object() || it->is_array()) stack.push_back(&(*it));
                }
            } else if (cur->is_array()) {
                for (const auto& v : *cur) {
                    if (v.is_object() || v.is_array()) stack.push_back(&v);
                }
            }
        }
    } catch (...) {
        // Ignore parse errors for non-standard JSON files
    }
}

static void normalizeVector(std::vector<float>& vec) {
    double ss = 0.0;
    for (float v : vec) ss += static_cast<double>(v) * v;
    if (ss <= 0.0) return;
    float inv = static_cast<float>(1.0 / std::sqrt(ss));
    for (float& v : vec) v *= inv;
}

static void seedFromSkdEmbeddings(SemanticDatabase& db, int dim, int& storedCount) {
    fs::path skdPath = fs::path("data") / "skd_embeddings.json";
    if (!fs::exists(skdPath)) return;
    try {
        std::ifstream f(skdPath);
        if (!f.is_open()) return;
        json j; f >> j;
        for (auto it = j.begin(); it != j.end(); ++it) {
            const std::string tag = it.key();
            std::vector<float> vec;
            if (it.value().is_array()) {
                for (const auto& x : it.value()) vec.push_back(x.get<float>());
            }
            if (vec.empty()) continue;
            if (static_cast<int>(vec.size()) != dim) vec.resize(dim, 0.0f);
            normalizeVector(vec);
            if (db.storeEmbedding(tag, vec, tag)) storedCount++;
        }
    } catch (...) {
        // ignore
    }
}

static void computeAndStoreIDF(SemanticDatabase& db,
                               const std::vector<std::vector<std::string>>& documents) {
    if (documents.empty()) return;
    std::unordered_map<std::string, int> docFreq;
    for (const auto& doc : documents) {
        std::unordered_set<std::string> unique(doc.begin(), doc.end());
        for (const auto& t : unique) docFreq[t]++;
    }
    const int totalDocs = static_cast<int>(documents.size());
    for (const auto& [tag, df] : docFreq) {
        float idf = std::log(static_cast<float>(totalDocs) / std::max(1, df));
        db.storeIDF(tag, idf, df);
    }
}

int main(int argc, char** argv) {
    try {
        Options opt;
        // Default JSON sources present in repo (best-effort)
        std::vector<std::string> defaultJsons = {
            "data/clean_config.json", // primary
            "clean_config.json",
            "group.json",
            "guitar.json",
            "structure.json",
            "moods.json",
            "Synthesizer.json"
        };

        // Parse args
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--help" || arg == "-h") {
                printUsage(argv[0]);
                return 0;
            } else if (arg == "--out" && i + 1 < argc) {
                opt.outDbPath = argv[++i];
            } else if (arg == "--dim" && i + 1 < argc) {
                opt.dimension = std::max(8, std::stoi(argv[++i]));
            } else if (arg == "--json" && i + 1 < argc) {
                opt.jsonPaths.push_back(argv[++i]);
            } else if (arg == "--no-md") {
                opt.includeMarkdown = false;
            } else {
                std::cerr << "Unknown option: " << arg << "\n";
                printUsage(argv[0]);
                return 1;
            }
        }

        // Ensure data directory exists
        try { fs::create_directories(fs::path(opt.outDbPath).parent_path()); } catch (...) {}

        // Create database and schema
        SemanticDatabase db(opt.outDbPath);
        if (!db.initializeSchema()) {
            std::cerr << "Failed to initialize schema in " << opt.outDbPath << "\n";
            return 2;
        }

        // Store config tunables used by contrastive query
        db.storeConfig("contrastive_alpha", 0.5f);
        db.storeConfig("contrastive_beta", 1.0f);
        // Optionally store scoring weights if available
        try {
            fs::path weightsPath = fs::path("config") / "weights.json";
            if (fs::exists(weightsPath)) {
                std::ifstream wf(weightsPath);
                if (wf.is_open()) {
                    json wj; wf >> wj;
                    if (wj.contains("weights") && wj["weights"].is_object()) {
                        auto& w = wj["weights"];
                        if (w.contains("semantic")) db.storeConfig("weight_semantic", w["semantic"].get<float>());
                        if (w.contains("technical")) db.storeConfig("weight_technical", w["technical"].get<float>());
                        if (w.contains("musicalRole")) db.storeConfig("weight_musical", w["musicalRole"].get<float>());
                        if (w.contains("layering")) db.storeConfig("weight_layering", w["layering"].get<float>());
                    }
                }
            }
        } catch (...) {}

        // Encoder (hash-based fallback) with fixed dimension for seed
        auto encoder = SentenceEncoder::createHashEncoder(&db, opt.dimension);

        // Collect tags from sources
        std::set<std::string> tagSet;

        // JSON sources
        for (const auto& p : defaultJsons) {
            if (fs::exists(p)) extractTokensFromJsonFile(p, tagSet);
        }
        for (const auto& p : opt.jsonPaths) {
            if (fs::exists(p)) extractTokensFromJsonFile(p, tagSet);
        }

        // Markdown sources
        if (opt.includeMarkdown) {
            extractTokensFromMarkdown(fs::current_path(), tagSet);
        }

        if (tagSet.empty()) {
            std::cerr << "No tags discovered. Aborting.\n";
            return 3;
        }

        // Seed curated embeddings first (if present), then hash for the rest
        int stored = 0;
        seedFromSkdEmbeddings(db, opt.dimension, stored);

        // Insert embeddings for all discovered tags
        for (const auto& tag : tagSet) {
            auto vec = encoder->encode(tag);
            if (vec.size() != static_cast<size_t>(opt.dimension)) {
                vec.resize(opt.dimension, 0.0f);
            }
            // Ensure unit-norm
            normalizeVector(vec);
            // storeEmbedding also inserts into tags table
            if (db.storeEmbedding(tag, vec, tag)) stored++;
        }

        // Compute IDF statistics if we have a clean config database
        std::vector<std::vector<std::string>> idfDocuments;
        try {
            fs::path cfg = fs::path("data") / "clean_config.json";
            if (fs::exists(cfg)) {
                std::ifstream f(cfg);
                json dbj; f >> dbj;
                for (const auto& [cfgId, cfgObj] : dbj.items()) {
                    std::vector<std::string> doc;
                    if (cfgObj.contains("soundCharacteristics")) {
                        const auto& sc = cfgObj["soundCharacteristics"];
                        if (sc.contains("timbral") && sc["timbral"].is_string()) {
                            auto toks = TextUtils::tokenize(sc["timbral"].get<std::string>());
                            doc.insert(doc.end(), toks.begin(), toks.end());
                        }
                        if (sc.contains("dynamic") && sc["dynamic"].is_string()) {
                            auto toks = TextUtils::tokenize(sc["dynamic"].get<std::string>());
                            doc.insert(doc.end(), toks.begin(), toks.end());
                        }
                        if (sc.contains("material") && sc["material"].is_string()) {
                            auto toks = TextUtils::tokenize(sc["material"].get<std::string>());
                            doc.insert(doc.end(), toks.begin(), toks.end());
                        }
                        if (sc.contains("emotional") && sc["emotional"].is_array()) {
                            for (const auto& e : sc["emotional"]) {
                                if (e.is_object() && e.contains("tag") && e["tag"].is_string()) {
                                    auto toks = TextUtils::tokenize(e["tag"].get<std::string>());
                                    doc.insert(doc.end(), toks.begin(), toks.end());
                                }
                            }
                        }
                    }
                    if (!doc.empty()) idfDocuments.push_back(std::move(doc));
                }
            }
        } catch (...) {}

        if (!idfDocuments.empty()) computeAndStoreIDF(db, idfDocuments);

        // Final report
        std::cout << "Seed completed: " << stored << " tags → " << opt.outDbPath << "\n";
        std::cout << "Dimension: " << opt.dimension << "\n";
        std::cout << "Markdown scanned: " << (opt.includeMarkdown ? "yes" : "no") << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Seeder error: " << e.what() << "\n";
        return 10;
    }
}
