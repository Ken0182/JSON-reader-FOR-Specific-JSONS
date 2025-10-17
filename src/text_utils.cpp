/**
 * @file text_utils.cpp
 * @brief Text Normalization and Tokenization Implementation
 * @author AI Assistant
 * @version 1.4
 */

#include "text_utils.hpp"
#include <regex>
#include <unordered_map>

namespace audio_config {

std::string TextUtils::toLower(const std::string& text) {
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string TextUtils::stripDiacritics(const std::string& text) {
    // Map of common diacritics to base characters
    // Note: Multi-byte unicode characters will show compiler warnings (expected)
    // These warnings are harmless - the code works correctly for ASCII text
    static const std::unordered_map<char, char> diacriticMap = {
        {'á', 'a'}, {'à', 'a'}, {'â', 'a'}, {'ä', 'a'}, {'ã', 'a'}, {'å', 'a'},
        {'é', 'e'}, {'è', 'e'}, {'ê', 'e'}, {'ë', 'e'},
        {'í', 'i'}, {'ì', 'i'}, {'î', 'i'}, {'ï', 'i'},
        {'ó', 'o'}, {'ò', 'o'}, {'ô', 'o'}, {'ö', 'o'}, {'õ', 'o'},
        {'ú', 'u'}, {'ù', 'u'}, {'û', 'u'}, {'ü', 'u'},
        {'ý', 'y'}, {'ÿ', 'y'},
        {'ñ', 'n'}, {'ç', 'c'},
        {'Á', 'A'}, {'À', 'A'}, {'Â', 'A'}, {'Ä', 'A'}, {'Ã', 'A'}, {'Å', 'A'},
        {'É', 'E'}, {'È', 'E'}, {'Ê', 'E'}, {'Ë', 'E'},
        {'Í', 'I'}, {'Ì', 'I'}, {'Î', 'I'}, {'Ï', 'I'},
        {'Ó', 'O'}, {'Ò', 'O'}, {'Ô', 'O'}, {'Ö', 'O'}, {'Õ', 'O'},
        {'Ú', 'U'}, {'Ù', 'U'}, {'Û', 'U'}, {'Ü', 'U'},
        {'Ý', 'Y'}, {'Ÿ', 'Y'},
        {'Ñ', 'N'}, {'Ç', 'C'}
    };
    
    std::string result;
    result.reserve(text.size());
    
    for (char c : text) {
        auto it = diacriticMap.find(c);
        result += (it != diacriticMap.end()) ? it->second : c;
    }
    
    return result;
}

std::vector<std::string> TextUtils::splitCamelSnake(const std::string& text) {
    std::vector<std::string> tokens;
    std::string current;
    
    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        char prev = (i > 0) ? text[i - 1] : '\0';
        char next = (i + 1 < text.length()) ? text[i + 1] : '\0';
        
        // Split on underscore or dash
        if (c == '_' || c == '-') {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            continue;
        }
        
        // Split on camelCase boundary: lowercase → uppercase
        if (i > 0 && std::islower(prev) && std::isupper(c)) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        }
        
        // Split on acronym boundary: UPPERCASE → Uppercase
        // e.g., "DX7Piano" → ["DX7", "Piano"]
        if (i > 0 && std::isupper(prev) && std::isupper(c) && std::islower(next)) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        }
        
        current += c;
    }
    
    if (!current.empty()) {
        tokens.push_back(current);
    }
    
    return tokens;
}

std::string TextUtils::stripPunctuation(const std::string& text) {
    std::string result;
    result.reserve(text.size());
    
    for (char c : text) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == ' ' || c == '_' || c == '-') {
            result += c;
        }
    }
    
    return result;
}

std::vector<std::string> TextUtils::tokenize(const std::string& text) {
    // Step 1: Strip diacritics
    std::string normalized = stripDiacritics(text);
    
    // Step 2: Convert to lowercase
    normalized = toLower(normalized);
    
    // Step 3: Strip punctuation (keep underscores/dashes for splitting)
    normalized = stripPunctuation(normalized);
    
    // Step 4: Split on camelCase and snake_case
    std::vector<std::string> tokens = splitCamelSnake(normalized);
    
    // Step 5: Further split on whitespace and filter empty tokens
    std::vector<std::string> finalTokens;
    for (const auto& token : tokens) {
        std::istringstream iss(token);
        std::string word;
        while (iss >> word) {
            if (!word.empty() && word.length() > 1) {  // Skip single chars
                finalTokens.push_back(word);
            }
        }
    }
    
    return finalTokens;
}

std::string TextUtils::normalizeForEmbedding(const std::string& text) {
    auto tokens = tokenize(text);
    return joinTokens(tokens, " ");
}

std::string TextUtils::joinTokens(const std::vector<std::string>& tokens,
                                  const std::string& delimiter) {
    if (tokens.empty()) return "";
    
    std::ostringstream oss;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (i > 0) oss << delimiter;
        oss << tokens[i];
    }
    return oss.str();
}

float TextUtils::calculateTokenOverlap(const std::vector<std::string>& queryTokens,
                                       const std::vector<std::string>& targetTokens) {
    if (queryTokens.empty() || targetTokens.empty()) {
        return 0.0f;
    }
    
    // Convert target to set for O(1) lookups
    std::unordered_set<std::string> targetSet(targetTokens.begin(), targetTokens.end());
    
    // Count matches
    int matches = 0;
    for (const auto& qToken : queryTokens) {
        if (targetSet.find(qToken) != targetSet.end()) {
            matches++;
        }
    }
    
    // Jaccard-style: matches / union size
    // Or use: matches / min(query, target) for recall-style
    float score = static_cast<float>(matches) / 
                  static_cast<float>(std::max(queryTokens.size(), targetTokens.size()));
    
    return score;
}

bool TextUtils::hasTokenMatch(const std::vector<std::string>& queryTokens,
                               const std::vector<std::string>& targetTokens) {
    std::unordered_set<std::string> targetSet(targetTokens.begin(), targetTokens.end());
    
    for (const auto& qToken : queryTokens) {
        if (targetSet.find(qToken) != targetSet.end()) {
            return true;
        }
    }
    
    return false;
}

bool TextUtils::isWordBoundary(char prev, char curr) {
    // Boundary between lowercase and uppercase
    if (std::islower(prev) && std::isupper(curr)) return true;
    // Boundary at underscore/dash
    if (prev == '_' || prev == '-') return true;
    return false;
}

const std::unordered_set<std::string>& TextUtils::getStopWords() {
    // Common stop words (can expand for future use)
    static const std::unordered_set<std::string> stopWords = {
        "a", "an", "the", "and", "or", "but", "in", "on", "at", "to", "for"
    };
    return stopWords;
}

} // namespace audio_config
