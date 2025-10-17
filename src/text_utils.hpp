/**
 * @file text_utils.hpp
 * @brief Shared Text Normalization and Tokenization Utilities
 * @author AI Assistant
 * @version 1.4
 * 
 * Unified tokenization pipeline for identifiers, tags, and queries.
 * Ensures consistent normalization across embedding generation and search.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <cctype>
#include <locale>
#include <sstream>

namespace audio_config {

/**
 * @brief Shared tokenization and normalization utilities
 * 
 * All text processing (IDs, tags, queries, embeddings) uses this
 * unified pipeline to ensure consistency between search and semantics.
 */
class TextUtils {
public:
    /**
     * @brief Normalize and tokenize text into canonical tokens
     * @param text Input text (ID, tag, query, etc.)
     * @return Vector of normalized tokens
     * 
     * Pipeline:
     * 1. Strip diacritics/accents
     * 2. Convert to lowercase
     * 3. Split camelCase and snake_case
     * 4. Strip punctuation
     * 5. Remove empty tokens
     * 6. Unicode fold (optional)
     */
    static std::vector<std::string> tokenize(const std::string& text);
    
    /**
     * @brief Convert text to lowercase
     * @param text Input text
     * @return Lowercase text
     */
    static std::string toLower(const std::string& text);
    
    /**
     * @brief Strip diacritics and accents
     * @param text Input text with potential diacritics
     * @return Text with diacritics removed
     * 
     * Example: "résumé" → "resume", "naïve" → "naive"
     */
    static std::string stripDiacritics(const std::string& text);
    
    /**
     * @brief Split camelCase and snake_case into separate tokens
     * @param text Input text (e.g., "Lead_Minimoog_RetroFunky")
     * @return Vector of split tokens (e.g., ["Lead", "Minimoog", "Retro", "Funky"])
     */
    static std::vector<std::string> splitCamelSnake(const std::string& text);
    
    /**
     * @brief Remove punctuation from text
     * @param text Input text
     * @return Text without punctuation
     */
    static std::string stripPunctuation(const std::string& text);
    
    /**
     * @brief Calculate token overlap score
     * @param queryTokens Query tokens (normalized)
     * @param targetTokens Target tokens (normalized)
     * @return Overlap score [0,1] based on shared tokens
     */
    static float calculateTokenOverlap(const std::vector<std::string>& queryTokens,
                                       const std::vector<std::string>& targetTokens);
    
    /**
     * @brief Check if any query token matches any target token
     * @param queryTokens Query tokens
     * @param targetTokens Target tokens
     * @return True if at least one token matches
     */
    static bool hasTokenMatch(const std::vector<std::string>& queryTokens,
                              const std::vector<std::string>& targetTokens);
    
    /**
     * @brief Join tokens back into a single string
     * @param tokens Vector of tokens
     * @param delimiter Delimiter (default: space)
     * @return Joined string
     */
    static std::string joinTokens(const std::vector<std::string>& tokens,
                                  const std::string& delimiter = " ");
    
    /**
     * @brief Normalize text for embedding generation
     * @param text Input text
     * @return Normalized string ready for embedding
     * 
     * Same normalization as tokenize() but returns single string
     */
    static std::string normalizeForEmbedding(const std::string& text);

private:
    // Helper: Check if character is word boundary for camelCase split
    static bool isWordBoundary(char prev, char curr);
    
    // Helper: Remove stop words (optional, for future use)
    static const std::unordered_set<std::string>& getStopWords();
};

} // namespace audio_config
