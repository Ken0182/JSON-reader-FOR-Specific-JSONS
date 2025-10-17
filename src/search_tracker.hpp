/**
 * @file search_tracker.hpp
 * @brief Persistent Search Interest Tracking with Decay and Smoothing
 * @author AI Assistant
 * @version 1.5
 * 
 * Tracks user search interests over time with temporal decay and smoothing.
 * Provides gentle bias signals for semantic search scoring.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <memory>
#include "json.hpp"

namespace audio_config {

/**
 * @brief Tunable parameters for search interest tracking
 */
struct SignalParameters {
    // Decay parameters
    float decayHalfLife{7200.0f};        // Half-life in seconds (default: 2 hours)
    float minSignalStrength{0.01f};      // Minimum signal to keep (prune below this)
    
    // Smoothing parameters
    float smoothingAlpha{0.3f};          // EMA smoothing factor (0=no smooth, 1=instant)
    
    // Bias parameters
    float biasStrength{0.2f};            // How much to bias search (0=none, 1=full)
    float biasClampMax{0.15f};           // Maximum bias contribution to score
    
    // History parameters
    int maxHistorySize{1000};            // Maximum query history to keep
    bool enableTracking{true};           // Master on/off switch
    
    /**
     * @brief Validate parameters are in reasonable ranges
     * @return True if valid
     */
    bool isValid() const noexcept;
    
    /**
     * @brief Load from JSON
     */
    static SignalParameters fromJson(const nlohmann::json& j);
    
    /**
     * @brief Save to JSON
     */
    nlohmann::json toJson() const;
};

/**
 * @brief Single search query record with timestamp
 */
struct QueryRecord {
    std::vector<std::string> tokens;                    // Normalized query tokens
    std::chrono::system_clock::time_point timestamp;    // When queried
    float rawStrength{1.0f};                            // Initial signal strength
    
    /**
     * @brief Calculate decayed signal strength
     * @param now Current time
     * @param decayHalfLife Half-life for exponential decay
     * @return Decayed strength [0,1]
     */
    float getDecayedStrength(const std::chrono::system_clock::time_point& now,
                             float decayHalfLife) const noexcept;
    
    /**
     * @brief Age of this query in seconds
     */
    float getAgeSeconds(const std::chrono::system_clock::time_point& now) const noexcept;
    
    nlohmann::json toJson() const;
    static QueryRecord fromJson(const nlohmann::json& j);
};

/**
 * @brief Persistent search interest tracker with temporal decay
 * 
 * Tracks which terms/tokens the user searches for, applies exponential
 * decay over time, and provides bias signals for search scoring.
 */
class SearchInterestTracker {
public:
    /**
     * @brief Constructor with parameters
     */
    explicit SearchInterestTracker(const SignalParameters& params = SignalParameters());
    
    /**
     * @brief Record a search query
     * @param tokens Normalized query tokens
     * 
     * Updates internal state with exponential moving average smoothing
     */
    void recordQuery(const std::vector<std::string>& tokens);
    
    /**
     * @brief Get bias signal for a set of tokens
     * @param tokens Query tokens to evaluate
     * @return Bias strength [0, biasClampMax], indicating user interest
     * 
     * Applies decay, smoothing, and clamping to produce gentle bias
     */
    float getBiasSignal(const std::vector<std::string>& tokens) const noexcept;
    
    /**
     * @brief Get current signal strength for a specific token
     * @param token Token to query
     * @return Decayed signal strength [0,1]
     */
    float getTokenSignal(const std::string& token) const noexcept;
    
    /**
     * @brief Get all active signals (decayed, above threshold)
     * @return Map of token → decayed strength
     */
    std::unordered_map<std::string, float> getActiveSignals() const;
    
    /**
     * @brief Get query history (most recent first)
     * @param maxResults Maximum number of queries to return
     * @return Vector of query records
     */
    std::vector<QueryRecord> getHistory(int maxResults = 50) const;
    
    /**
     * @brief Clear all tracked data
     */
    void clear();
    
    /**
     * @brief Prune old/weak signals below threshold
     * 
     * Removes signals that have decayed below minSignalStrength
     */
    void pruneWeakSignals();
    
    /**
     * @brief Enable/disable tracking
     */
    void setEnabled(bool enabled) { params_.enableTracking = enabled; }
    
    /**
     * @brief Check if tracking is enabled
     */
    bool isEnabled() const noexcept { return params_.enableTracking; }
    
    /**
     * @brief Get current parameters
     */
    const SignalParameters& getParameters() const noexcept { return params_; }
    
    /**
     * @brief Update parameters
     */
    void setParameters(const SignalParameters& params);
    
    /**
     * @brief Export state to JSON
     * @return JSON object with full state
     */
    nlohmann::json exportState() const;
    
    /**
     * @brief Import state from JSON
     * @param j JSON object with state
     * @return True if import succeeded
     */
    bool importState(const nlohmann::json& j);
    
    /**
     * @brief Get statistics about tracker state
     * @return Map of stat_name → value
     */
    std::unordered_map<std::string, float> getStatistics() const;

private:
    SignalParameters params_;
    
    // Token → {strength, last_update_time} for EMA smoothing
    std::unordered_map<std::string, std::pair<float, std::chrono::system_clock::time_point>> tokenSignals_;
    
    // Query history (most recent last)
    std::vector<QueryRecord> queryHistory_;
    
    /**
     * @brief Update signal for a token with EMA smoothing
     */
    void updateTokenSignal(const std::string& token, float newStrength);
    
    /**
     * @brief Calculate exponential decay factor
     */
    float calculateDecayFactor(float ageSeconds) const noexcept;
};

} // namespace audio_config
