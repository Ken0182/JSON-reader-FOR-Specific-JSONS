/**
 * @file contrastive_query.cpp
 * @brief Contrastive Query Implementation
 * @author AI Assistant
 * @version 1.6
 */

#include "contrastive_query.hpp"
#include "audio_config_system.hpp"
#include "text_utils.hpp"
#include <algorithm>
#include <sstream>
#include <regex>

namespace audio_config {

std::string QuerySpec::getIncludeText() const {
    std::string result;
    for (size_t i = 0; i < includeTerms.size(); ++i) {
        if (i > 0) result += " ";
        result += includeTerms[i];
    }
    return result;
}

std::string QuerySpec::getExcludeText() const {
    std::string result;
    for (size_t i = 0; i < excludeTerms.size(); ++i) {
        if (i > 0) result += " ";
        result += excludeTerms[i];
    }
    return result;
}

ContrastiveParams ContrastiveParams::loadFromDB(const SemanticDatabase* db) {
    ContrastiveParams params;
    if (db) {
        params.alphaExclude = db->getConfigValue("contrastive_alpha", 0.5f);
        params.betaPositive = db->getConfigValue("contrastive_beta", 1.0f);
    }
    return params;
}

QuerySpec parseContrastiveQuery(const std::string& query) {
    QuerySpec spec;
    
    if (query.empty()) {
        return spec;
    }
    
    std::string lowerQuery = TextUtils::toLower(query);
    
    // Negation patterns (in order of precedence)
    std::vector<std::string> negationPatterns = {
        " but not ",
        " without ",
        " excluding ",
        " avoid ",
        " not "
    };
    
    std::string includeText = lowerQuery;
    std::string excludeText;
    
    // Find first matching negation pattern
    for (const auto& pattern : negationPatterns) {
        size_t pos = lowerQuery.find(pattern);
        if (pos != std::string::npos) {
            // Split at negation
            includeText = lowerQuery.substr(0, pos);
            excludeText = lowerQuery.substr(pos + pattern.length());
            break;
        }
    }
    
    // Tokenize include terms
    auto includeTokens = TextUtils::tokenize(includeText);
    spec.includeTerms = includeTokens;
    
    // Tokenize exclude terms
    if (!excludeText.empty()) {
        auto excludeTokens = TextUtils::tokenize(excludeText);
        spec.excludeTerms = excludeTokens;
    }
    
    return spec;
}

std::vector<float> computeContrastiveVector(
    const QuerySpec& spec,
    const EmbeddingEngine& engine,
    const ContrastiveParams& params) {
    
    int dim = engine.getDimension();
    std::vector<float> queryVec(dim, 0.0f);
    
    // Get include embedding
    if (!spec.includeTerms.empty()) {
        std::string includeText = spec.getIncludeText();
        auto includeVec = engine.getEmbedding(includeText);
        
        // Add with beta weight
        for (int i = 0; i < dim && i < static_cast<int>(includeVec.size()); ++i) {
            queryVec[i] += params.betaPositive * includeVec[i];
        }
    }
    
    // Subtract exclude embedding
    if (!spec.excludeTerms.empty()) {
        std::string excludeText = spec.getExcludeText();
        auto excludeVec = engine.getEmbedding(excludeText);
        
        // Subtract with alpha weight
        for (int i = 0; i < dim && i < static_cast<int>(excludeVec.size()); ++i) {
            queryVec[i] -= params.alphaExclude * excludeVec[i];
        }
    }
    
    // If no include terms, use neutral vector
    if (spec.includeTerms.empty()) {
        float neutral = 1.0f / std::sqrt(static_cast<float>(dim));
        for (int i = 0; i < dim; ++i) {
            queryVec[i] = neutral;
        }
    }
    
    // Normalize result
    EmbeddingEngine::normalizeEmbedding(queryVec);
    
    return queryVec;
}

} // namespace audio_config
