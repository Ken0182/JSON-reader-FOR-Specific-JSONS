/**
 * @file contrastive_query.hpp
 * @brief Contrastive Query Composition - "X but not Y" Support
 * @author AI Assistant
 * @version 1.6
 * 
 * v1.6 Contrastive Queries:
 * - Parse natural language negation syntax
 * - Compose query vectors with positive and negative constraints
 * - Formula: q = normalize(include + β·pos - α·neg)
 * - Handles: "but not", "without", "avoid", "not"
 * 
 * USAGE:
 *   QuerySpec spec = parseContrastiveQuery("dreamy but not lush");
 *   // spec.includeTerms = ["dreamy"]
 *   // spec.excludeTerms = ["lush"]
 *   
 *   auto queryVec = computeContrastiveVector(spec, encoder);
 *   // queryVec = normalize(embed("dreamy") - α·embed("lush"))
 * 
 * EXAMPLES:
 *   "dreamy but not lush"       → include: dreamy, exclude: lush
 *   "bright without harsh"      → include: bright, exclude: harsh
 *   "analog not digital"        → include: analog, exclude: digital
 *   "warm pad avoid metallic"   → include: warm pad, exclude: metallic
 */

#pragma once

#include <string>
#include <vector>

namespace audio_config {

// Forward declarations
class EmbeddingEngine;

/**
 * @brief Parsed query specification with positive and negative constraints
 */
struct QuerySpec {
    std::vector<std::string> includeTerms;  // Positive constraints
    std::vector<std::string> excludeTerms;   // Negative constraints (to subtract)
    
    /**
     * @brief Check if query has negative constraints
     */
    bool hasNegatives() const { return !excludeTerms.empty(); }
    
    /**
     * @brief Get full include text (joined)
     */
    std::string getIncludeText() const;
    
    /**
     * @brief Get full exclude text (joined)
     */
    std::string getExcludeText() const;
};

/**
 * @brief Parameters for contrastive query composition
 */
struct ContrastiveParams {
    float alphaExclude{0.5f};   // Negative term weight (default: 0.5)
    float betaPositive{1.0f};   // Positive term weight (default: 1.0)
    
    /**
     * @brief Load from database config table
     */
    static ContrastiveParams loadFromDB(const class SemanticDatabase* db);
};

/**
 * @brief Parse natural language query into contrastive spec
 * @param query User query string
 * @return Parsed query specification
 * 
 * Recognizes patterns:
 * - "X but not Y"
 * - "X without Y"
 * - "X avoid Y"
 * - "X not Y"
 * - "X excluding Y"
 */
QuerySpec parseContrastiveQuery(const std::string& query);

/**
 * @brief Compute contrastive query vector
 * @param spec Parsed query specification
 * @param engine Embedding engine
 * @param params Contrastive parameters (optional)
 * @return Unit-normalized query vector
 * 
 * Formula: q = normalize(β·include - α·exclude)
 * Where α controls negative term strength (default 0.5)
 */
std::vector<float> computeContrastiveVector(
    const QuerySpec& spec,
    const EmbeddingEngine& engine,
    const ContrastiveParams& params = ContrastiveParams());

} // namespace audio_config
