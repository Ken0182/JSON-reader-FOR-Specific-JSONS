#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <filesystem>
#include <cmath>
#include <algorithm>

#include "json.hpp"
#include "text_utils.hpp"
#include "semantic_db.hpp"
#include "sentence_encoder.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;
using namespace audio_config;

struct SeederOptions {
    std::string dbPath{"data/semantic.db"};
    int dimension{100};
    bool verbose{true};
};

static bool loadJsonFile(const fs::path& path, json& out) {
    try {
        std::ifstream f(path);
        if (!f.is_open()) return false;
        f >> out;
        return true;
    } catch (...) {
        return false;
    }
}

static void addIfNonEmpty(const std::string& s, std::unordered_set<std::string>& outTags) {
    if (s.empty()) return;
    auto tokens = TextUtils::tokenize(s);
    for (const auto& t : tokens) {
        if (!t.empty()) outTags.insert(t);
    }
}

static void collectTagsFromGroupJson(const json& j, std::unordered_set<std::string>& outTags, std::vector<std::unordered_set<std::string>>& documents) {
    if (!j.contains("groups") || !j["groups"].is_object()) return;
    for (auto it = j["groups"].begin(); it != j["groups"].end(); ++it) {
        const auto& group = it.value();
        std::unordered_set<std::string> docTags;
        if (group.contains("sound_characteristics") && group["sound_characteristics"].is_object()) {
            const auto& sc = group["sound_characteristics"];
            if (sc.contains("timbral") && sc["timbral"].is_string()) {
                addIfNonEmpty(sc["timbral"].get<std::string>(), outTags);
                addIfNonEmpty(sc["timbral"].get<std::string>(), docTags);
            }
            if (sc.contains("dynamic") && sc["dynamic"].is_string()) {
                addIfNonEmpty(sc["dynamic"].get<std::string>(), outTags);
                addIfNonEmpty(sc["dynamic"].get<std::string>(), docTags);
            }
            if (sc.contains("material") && sc["material"].is_string()) {
                addIfNonEmpty(sc["material"].get<std::string>(), outTags);
                addIfNonEmpty(sc["material"].get<std::string>(), docTags);
            }
            if (sc.contains("emotional") && sc["emotional"].is_array()) {
                for (const auto& e : sc["emotional"]) {
                    if (e.is_object() && e.contains("tag") && e["tag"].is_string()) {
                        addIfNonEmpty(e["tag"].get<std::string>(), outTags);
                        addIfNonEmpty(e["tag"].get<std::string>(), docTags);
                    }
                }
            }
        }
        if (!docTags.empty()) documents.push_back(std::move(docTags));
    }
}

static void collectTagsFromGuitarJson(const json& j, std::unordered_set<std::string>& outTags, std::vector<std::unordered_set<std::string>>& documents) {
    if (!j.contains("guitar_types") || !j["guitar_types"].is_object()) return;
    const auto& types = j["guitar_types"];
    for (auto t = types.begin(); t != types.end(); ++t) {
        if (!t.value().contains("groups")) continue;
        const auto& groups = t.value()["groups"];
        for (auto g = groups.begin(); g != groups.end(); ++g) {
            const auto& group = g.value();
            std::unordered_set<std::string> docTags;
            if (group.contains("sound_characteristics") && group["sound_characteristics"].is_object()) {
                const auto& sc = group["sound_characteristics"];
                if (sc.contains("timbral") && sc["timbral"].is_string()) {
                    addIfNonEmpty(sc["timbral"].get<std::string>(), outTags);
                    addIfNonEmpty(sc["timbral"].get<std::string>(), docTags);
                }
                if (sc.contains("dynamic") && sc["dynamic"].is_string()) {
                    addIfNonEmpty(sc["dynamic"].get<std::string>(), outTags);
                    addIfNonEmpty(sc["dynamic"].get<std::string>(), docTags);
                }
                if (sc.contains("material") && sc["material"].is_string()) {
                    addIfNonEmpty(sc["material"].get<std::string>(), outTags);
                    addIfNonEmpty(sc["material"].get<std::string>(), docTags);
                }
                if (sc.contains("emotional") && sc["emotional"].is_array()) {
                    for (const auto& e : sc["emotional"]) {
                        if (e.is_object() && e.contains("tag") && e["tag"].is_string()) {
                            addIfNonEmpty(e["tag"].get<std::string>(), outTags);
                            addIfNonEmpty(e["tag"].get<std::string>(), docTags);
                        }
                    }
                }
            }
            if (!docTags.empty()) documents.push_back(std::move(docTags));
        }
    }
}

static void collectTagsFromCleanConfig(const json& j, std::unordered_set<std::string>& outTags, std::vector<std::unordered_set<std::string>>& documents) {
    if (!j.is_object()) return;
    for (auto it = j.begin(); it != j.end(); ++it) {
        const auto& config = it.value();
        std::unordered_set<std::string> docTags;
        // v1.2 style: soundCharacteristics
        if (config.contains("soundCharacteristics") && config["soundCharacteristics"].is_object()) {
            const auto& sc = config["soundCharacteristics"];
            if (sc.contains("timbral") && sc["timbral"].is_string()) {
                addIfNonEmpty(sc["timbral"].get<std::string>(), outTags);
                addIfNonEmpty(sc["timbral"].get<std::string>(), docTags);
            }
            if (sc.contains("dynamic") && sc["dynamic"].is_string()) {
                addIfNonEmpty(sc["dynamic"].get<std::string>(), outTags);
                addIfNonEmpty(sc["dynamic"].get<std::string>(), docTags);
            }
            if (sc.contains("material") && sc["material"].is_string()) {
                addIfNonEmpty(sc["material"].get<std::string>(), outTags);
                addIfNonEmpty(sc["material"].get<std::string>(), docTags);
            }
            if (sc.contains("emotional") && sc["emotional"].is_array()) {
                for (const auto& e : sc["emotional"]) {
                    if (e.is_object() && e.contains("tag") && e["tag"].is_string()) {
                        addIfNonEmpty(e["tag"].get<std::string>(), outTags);
                        addIfNonEmpty(e["tag"].get<std::string>(), docTags);
                    }
                }
            }
        }
        if (!docTags.empty()) documents.push_back(std::move(docTags));
    }
}

static void collectTokensFromMarkdown(const fs::path& root, std::unordered_set<std::string>& outTags) {
    for (auto it = fs::recursive_directory_iterator(root); it != fs::recursive_directory_iterator(); ++it) {
        if (!it->is_regular_file()) continue;
        auto p = it->path();
        if (p.extension() == ".md") {
            try {
                std::ifstream f(p);
                if (!f.is_open()) continue;
                std::stringstream buffer;
                buffer << f.rdbuf();
                auto tokens = TextUtils::tokenize(buffer.str());
                for (const auto& t : tokens) {
                    if (t.size() >= 3 && t.size() <= 24) {
                        outTags.insert(t);
                    }
                }
            } catch (...) {
                // ignore
            }
        }
    }
}

static std::unordered_map<std::string, std::string> buildAliasMap() {
    // Minimal, safe aliasing for canonicalization
    return std::unordered_map<std::string, std::string>{
        {"analogue", "analog"},
        {"synthesizer", "synth"},
        {"synths", "synth"},
        {"brilliant", "bright"},
        {"crisp", "bright"},
        {"clear", "bright"},
        {"sharp", "bright"},
        {"mellow", "warm"},
        {"soft", "warm"},
        {"smooth", "warm"},
        {"round", "warm"},
        {"airy", "dreamy"},
        {"ethereal", "dreamy"},
        {"retro", "vintage"},
        {"classic", "vintage"},
        {"analogue", "analog"},
        {"agressive", "aggressive"}
    };
}

static void ensureDir(const fs::path& p) {
    if (!fs::exists(p)) fs::create_directories(p);
}

static bool isLikelyDescriptor(const std::string& token) {
    // Heuristic: common audio descriptor vocabulary + alpha-only
    static const std::unordered_set<std::string> whitelist = {
        "warm","bright","dark","lush","dreamy","harsh","crisp","jangly","fuzzy","airy","ethereal",
        "mellow","punchy","aggressive","smooth","analog","digital","vintage","modern","fat","thin",
        "organic","synthetic","metallic","wooden","glassy","gritty","squelchy","clear","soft","sharp",
        "energetic","calm","peaceful","intense","powerful","bold","gentle","classic","retro","natural",
        "acoustic","electric","nostalgic","uplifting","playful","bouncy","chaotic","experimental","ambient",
        "plucky","percussive","sustained","delicate","driving","steady","unsettling","mystical","futuristic"
    };
    if (whitelist.count(token)) return true;
    // alpha-only heuristic
    return std::all_of(token.begin(), token.end(), [](char c){return std::isalpha(static_cast<unsigned char>(c));});
}

static void seedDatabase(const SeederOptions& opts) {
    ensureDir(fs::path(opts.dbPath).parent_path());

    SemanticDatabase db(opts.dbPath);
    db.initializeSchema();

    // Prepare encoder (hash-based, normalized outputs)
    auto encoder = SentenceEncoder::createHashEncoder(&db, opts.dimension);

    // Collect tags
    std::unordered_set<std::string> tags;
    std::vector<std::unordered_set<std::string>> documents; // for IDF

    // JSON sources
    json jGroup, jGuitar, jStructure, jClean;
    if (loadJsonFile("group.json", jGroup)) {
        collectTagsFromGroupJson(jGroup, tags, documents);
    }
    if (loadJsonFile("guitar.json", jGuitar)) {
        collectTagsFromGuitarJson(jGuitar, tags, documents);
    }
    if (loadJsonFile("structure.json", jStructure)) {
        // structure.json has sections and group refs; no direct tags; skip
        (void)jStructure;
    }
    if (fs::exists("data/clean_config.json") && loadJsonFile("data/clean_config.json", jClean)) {
        collectTagsFromCleanConfig(jClean, tags, documents);
    }

    // Markdown sources
    collectTokensFromMarkdown(fs::current_path(), tags);

    // Add required core tags to guarantee sensible default KB
    const std::vector<std::string> coreTags = {
        "warm","bright","dark","lush","dreamy","harsh","analog","digital","vintage","modern",
        "punchy","mellow","smooth","fat","thin","organic","synthetic","metallic","wooden","glassy",
        "gritty","squelchy","crisp","jangly","fuzzy","airy","ethereal","clear","soft","sharp"
    };
    tags.insert(coreTags.begin(), coreTags.end());

    // Alias mappings
    auto aliasMap = buildAliasMap();

    // First, create embeddings for canonical tags only
    std::unordered_set<std::string> canonicalTags;
    canonicalTags.reserve(tags.size());
    for (const auto& t : tags) {
        std::string canonical = t;
        auto it = aliasMap.find(t);
        if (it != aliasMap.end()) {
            canonical = it->second;
        }
        if (isLikelyDescriptor(canonical)) {
            canonicalTags.insert(canonical);
        }
    }

    // Store embeddings for canonical tags
    int stored = 0;
    for (const auto& tag : canonicalTags) {
        // Encode single-token string; HashEncoder returns unit-normalized
        auto emb = encoder->encode(tag);
        if (static_cast<int>(emb.size()) != opts.dimension) {
            emb.resize(opts.dimension, 0.0f);
            // re-normalize
            float ss = 0.0f; for (float v : emb) ss += v*v; if (ss > 1e-8f) { float n = std::sqrt(ss); for (float& v : emb) v/=n; }
        }
        if (db.storeEmbedding(tag, emb, tag)) {
            stored++;
        }
    }

    // Store aliases (rows in tags with canonical != tag)
    int aliasStored = 0;
    for (const auto& [alias, canonical] : aliasMap) {
        if (alias == canonical) continue;
        if (canonicalTags.count(canonical) == 0) continue; // ensure target exists
        if (db.storeAlias(alias, canonical)) aliasStored++;
    }

    // Compute IDF statistics from documents
    if (!documents.empty()) {
        std::unordered_map<std::string, int> docFreq;
        for (const auto& doc : documents) {
            std::unordered_set<std::string> uniqueInDoc;
            for (const auto& t : doc) {
                std::string c = t;
                auto it = aliasMap.find(t);
                if (it != aliasMap.end()) c = it->second;
                uniqueInDoc.insert(c);
            }
            for (const auto& c : uniqueInDoc) docFreq[c]++;
        }
        const int totalDocs = static_cast<int>(documents.size());
        for (const auto& [tag, df] : docFreq) {
            float idf = std::log(static_cast<float>(totalDocs) / std::max(1, df));
            db.storeIDF(tag, idf, df);
        }
    }

    // Store tunable config defaults
    db.storeConfig("contrastive_alpha", 0.5f);
    db.storeConfig("contrastive_beta", 1.0f);

    if (opts.verbose) {
        std::cout << "Seeded semantic DB at: " << opts.dbPath << "\n";
        std::cout << "  Dimension: " << opts.dimension << "\n";
        std::cout << "  Tags stored: " << stored << "\n";
        std::cout << "  Aliases stored: " << aliasStored << "\n";
    }
}

static SeederOptions parseArgs(int argc, char** argv) {
    SeederOptions opts;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--db" || arg == "-o") && i + 1 < argc) {
            opts.dbPath = argv[++i];
        } else if ((arg == "--dim" || arg == "-d") && i + 1 < argc) {
            try { opts.dimension = std::stoi(argv[++i]); } catch (...) {}
        } else if (arg == "--quiet" || arg == "-q") {
            opts.verbose = false;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: seed_semantic_db [--db path] [--dim D] [--quiet]\n";
            std::exit(0);
        }
    }
    return opts;
}

int main(int argc, char** argv) {
    try {
        auto opts = parseArgs(argc, argv);
        seedDatabase(opts);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Seeding error: " << e.what() << "\n";
        return 1;
    }
}
