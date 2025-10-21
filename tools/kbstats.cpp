#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cmath>
#include <filesystem>
#include "semantic_db.hpp"

using audio_config::SemanticDatabase;
namespace fs = std::filesystem;

static void printUsage(const char* prog) {
    std::cout << "kbstats: Inspect semantic.db integrity\n\n";
    std::cout << "Usage: " << prog << " [path/to/semantic.db]\n\n";
}

int main(int argc, char** argv) {
    std::string dbPath = "data/semantic.db";
    if (argc > 1) dbPath = argv[1];

    try {
        SemanticDatabase db(dbPath);
        if (!db.initializeSchema()) {
            std::cerr << "Failed to open or initialize DB: " << dbPath << "\n";
            return 2;
        }

        int dim = db.getEmbeddingDimension();
        auto tags = db.getAllTags();

        std::cout << "D=" << dim << ", tag_count=" << tags.size();
        // alias_count approximated as tags whose canonical != tag
        size_t aliasCount = 0;
        for (const auto& t : tags) {
            if (db.getCanonicalTag(t) != t) aliasCount++;
        }
        std::cout << ", alias_count=" << aliasCount << "\n";

        // Sample tags
        std::cout << "sample tags: ";
        for (size_t i = 0; i < tags.size() && i < 8; ++i) {
            if (i) std::cout << ", ";
            std::cout << tags[i];
        }
        std::cout << "\n";

        // Norm checks for a few embeddings
        size_t checked = 0; size_t nonUnit = 0;
        for (size_t i = 0; i < tags.size() && checked < 10; ++i) {
            auto v = db.getEmbedding(tags[i]);
            if (v.empty()) continue;
            double ss = 0.0; for (float x : v) ss += x * x; ss = std::sqrt(ss);
            if (std::abs(ss - 1.0) > 5e-2) nonUnit++;
            checked++;
        }
        std::cout << "unit_norm_check(nonunit/checked)=" << nonUnit << "/" << checked << "\n";

        // Sanity: query "dreamy not harsh" demo top-N by cosine to a few tags
        // Lightweight: just show cosine with some known descriptors
        std::vector<std::string> demo = {"dreamy", "harsh", "warm", "bright", "lush", "aggressive"};
        std::cout << "probe: ";
        for (const auto& p : demo) {
            auto e = db.getEmbedding(p);
            std::cout << p << "(" << (e.empty() ? 0 : (int)e.size()) << "D) ";
        }
        std::cout << "\n";

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "kbstats error: " << e.what() << "\n";
        return 1;
    }
}
