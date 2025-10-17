/**
 * @file search_tracker.cpp
 * @brief Search Interest Tracker Implementation
 * @author AI Assistant
 * @version 1.5
 */

#include "search_tracker.hpp"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace audio_config {

// ============================================================================
// SignalParameters Implementation
// ============================================================================

bool SignalParameters::isValid() const noexcept {
    return decayHalfLife > 0.0f &&
           minSignalStrength >= 0.0f && minSignalStrength < 1.0f &&
           smoothingAlpha >= 0.0f && smoothingAlpha <= 1.0f &&
           biasStrength >= 0.0f && biasStrength <= 1.0f &&
           biasClampMax >= 0.0f && biasClampMax <= 1.0f &&
           maxHistorySize > 0;
}

SignalParameters SignalParameters::fromJson(const nlohmann::json& j) {
    SignalParameters params;
    
    if (j.contains("decayHalfLife")) params.decayHalfLife = j["decayHalfLife"].get<float>();
    if (j.contains("minSignalStrength")) params.minSignalStrength = j["minSignalStrength"].get<float>();
    if (j.contains("smoothingAlpha")) params.smoothingAlpha = j["smoothingAlpha"].get<float>();
    if (j.contains("biasStrength")) params.biasStrength = j["biasStrength"].get<float>();
    if (j.contains("biasClampMax")) params.biasClampMax = j["biasClampMax"].get<float>();
    if (j.contains("maxHistorySize")) params.maxHistorySize = j["maxHistorySize"].get<int>();
    if (j.contains("enableTracking")) params.enableTracking = j["enableTracking"].get<bool>();
    
    return params;
}

nlohmann::json SignalParameters::toJson() const {
    nlohmann::json j;
    
    j["decayHalfLife"] = decayHalfLife;
    j["minSignalStrength"] = minSignalStrength;
    j["smoothingAlpha"] = smoothingAlpha;
    j["biasStrength"] = biasStrength;
    j["biasClampMax"] = biasClampMax;
    j["maxHistorySize"] = maxHistorySize;
    j["enableTracking"] = enableTracking;
    
    return j;
}

// ============================================================================
// QueryRecord Implementation
// ============================================================================

float QueryRecord::getDecayedStrength(const std::chrono::system_clock::time_point& now,
                                      float decayHalfLife) const noexcept {
    float ageSeconds = getAgeSeconds(now);
    
    // Exponential decay: strength * (0.5)^(age / halfLife)
    float decayFactor = std::pow(0.5f, ageSeconds / decayHalfLife);
    
    return rawStrength * decayFactor;
}

float QueryRecord::getAgeSeconds(const std::chrono::system_clock::time_point& now) const noexcept {
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - timestamp);
    return static_cast<float>(duration.count());
}

nlohmann::json QueryRecord::toJson() const {
    nlohmann::json j;
    
    j["tokens"] = tokens;
    j["timestamp"] = std::chrono::system_clock::to_time_t(timestamp);
    j["rawStrength"] = rawStrength;
    
    return j;
}

QueryRecord QueryRecord::fromJson(const nlohmann::json& j) {
    QueryRecord record;
    
    if (j.contains("tokens")) {
        record.tokens = j["tokens"].get<std::vector<std::string>>();
    }
    
    if (j.contains("timestamp")) {
        auto time_t_val = j["timestamp"].get<std::time_t>();
        record.timestamp = std::chrono::system_clock::from_time_t(time_t_val);
    }
    
    if (j.contains("rawStrength")) {
        record.rawStrength = j["rawStrength"].get<float>();
    }
    
    return record;
}

// ============================================================================
// SearchInterestTracker Implementation
// ============================================================================

SearchInterestTracker::SearchInterestTracker(const SignalParameters& params)
    : params_(params) {
    
    if (!params_.isValid()) {
        params_ = SignalParameters();  // Fall back to defaults
    }
}

void SearchInterestTracker::recordQuery(const std::vector<std::string>& tokens) {
    if (!params_.enableTracking || tokens.empty()) {
        return;
    }
    
    auto now = std::chrono::system_clock::now();
    
    // Create query record
    QueryRecord record;
    record.tokens = tokens;
    record.timestamp = now;
    record.rawStrength = 1.0f;
    
    // Add to history
    queryHistory_.push_back(record);
    
    // Limit history size
    if (static_cast<int>(queryHistory_.size()) > params_.maxHistorySize) {
        queryHistory_.erase(queryHistory_.begin());
    }
    
    // Update token signals with EMA smoothing
    for (const auto& token : tokens) {
        updateTokenSignal(token, 1.0f);
    }
    
    // Periodically prune weak signals (every 10 queries)
    if (queryHistory_.size() % 10 == 0) {
        pruneWeakSignals();
    }
}

float SearchInterestTracker::getBiasSignal(const std::vector<std::string>& tokens) const noexcept {
    if (!params_.enableTracking || tokens.empty()) {
        return 0.0f;
    }
    
    auto now = std::chrono::system_clock::now();
    float totalSignal = 0.0f;
    int matchedTokens = 0;
    
    // Aggregate signals from matching tokens
    for (const auto& token : tokens) {
        float signal = getTokenSignal(token);
        if (signal > params_.minSignalStrength) {
            totalSignal += signal;
            matchedTokens++;
        }
    }
    
    if (matchedTokens == 0) {
        return 0.0f;
    }
    
    // Average signal across matched tokens
    float averageSignal = totalSignal / static_cast<float>(matchedTokens);
    
    // Apply bias strength multiplier
    float biasedSignal = averageSignal * params_.biasStrength;
    
    // Clamp to maximum bias contribution
    return std::min(biasedSignal, params_.biasClampMax);
}

float SearchInterestTracker::getTokenSignal(const std::string& token) const noexcept {
    auto it = tokenSignals_.find(token);
    if (it == tokenSignals_.end()) {
        return 0.0f;
    }
    
    auto now = std::chrono::system_clock::now();
    const auto& [strength, lastUpdate] = it->second;
    
    // Calculate age since last update
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate);
    float ageSeconds = static_cast<float>(duration.count());
    
    // Apply exponential decay
    float decayFactor = calculateDecayFactor(ageSeconds);
    
    return strength * decayFactor;
}

std::unordered_map<std::string, float> SearchInterestTracker::getActiveSignals() const {
    std::unordered_map<std::string, float> activeSignals;
    
    for (const auto& [token, signalData] : tokenSignals_) {
        float signal = getTokenSignal(token);
        if (signal > params_.minSignalStrength) {
            activeSignals[token] = signal;
        }
    }
    
    return activeSignals;
}

std::vector<QueryRecord> SearchInterestTracker::getHistory(int maxResults) const {
    std::vector<QueryRecord> history;
    
    // Return most recent queries first
    int startIdx = std::max(0, static_cast<int>(queryHistory_.size()) - maxResults);
    
    for (int i = static_cast<int>(queryHistory_.size()) - 1; i >= startIdx; --i) {
        history.push_back(queryHistory_[i]);
    }
    
    return history;
}

void SearchInterestTracker::clear() {
    tokenSignals_.clear();
    queryHistory_.clear();
}

void SearchInterestTracker::pruneWeakSignals() {
    auto now = std::chrono::system_clock::now();
    
    // Remove signals below threshold
    for (auto it = tokenSignals_.begin(); it != tokenSignals_.end();) {
        const auto& [strength, lastUpdate] = it->second;
        
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate);
        float ageSeconds = static_cast<float>(duration.count());
        float decayFactor = calculateDecayFactor(ageSeconds);
        float decayedStrength = strength * decayFactor;
        
        if (decayedStrength < params_.minSignalStrength) {
            it = tokenSignals_.erase(it);
        } else {
            ++it;
        }
    }
}

void SearchInterestTracker::setParameters(const SignalParameters& params) {
    if (params.isValid()) {
        params_ = params;
    }
}

nlohmann::json SearchInterestTracker::exportState() const {
    nlohmann::json state;
    
    // Export parameters
    state["parameters"] = params_.toJson();
    
    // Export token signals
    nlohmann::json signals = nlohmann::json::object();
    for (const auto& [token, signalData] : tokenSignals_) {
        const auto& [strength, lastUpdate] = signalData;
        nlohmann::json signalJson;
        signalJson["strength"] = strength;
        signalJson["timestamp"] = std::chrono::system_clock::to_time_t(lastUpdate);
        signals[token] = signalJson;
    }
    state["signals"] = signals;
    
    // Export query history
    nlohmann::json history = nlohmann::json::array();
    for (const auto& record : queryHistory_) {
        history.push_back(record.toJson());
    }
    state["history"] = history;
    
    return state;
}

bool SearchInterestTracker::importState(const nlohmann::json& j) {
    try {
        // Import parameters
        if (j.contains("parameters")) {
            params_ = SignalParameters::fromJson(j["parameters"]);
            if (!params_.isValid()) {
                return false;
            }
        }
        
        // Import token signals
        if (j.contains("signals")) {
            tokenSignals_.clear();
            for (auto& [token, signalJson] : j["signals"].items()) {
                float strength = signalJson["strength"].get<float>();
                auto time_t_val = signalJson["timestamp"].get<std::time_t>();
                auto timestamp = std::chrono::system_clock::from_time_t(time_t_val);
                
                tokenSignals_[token] = {strength, timestamp};
            }
        }
        
        // Import query history
        if (j.contains("history")) {
            queryHistory_.clear();
            for (const auto& recordJson : j["history"]) {
                queryHistory_.push_back(QueryRecord::fromJson(recordJson));
            }
        }
        
        return true;
        
    } catch (const std::exception&) {
        return false;
    }
}

std::unordered_map<std::string, float> SearchInterestTracker::getStatistics() const {
    std::unordered_map<std::string, float> stats;
    
    stats["total_queries"] = static_cast<float>(queryHistory_.size());
    stats["active_signals"] = static_cast<float>(getActiveSignals().size());
    stats["total_tokens_tracked"] = static_cast<float>(tokenSignals_.size());
    stats["tracking_enabled"] = params_.enableTracking ? 1.0f : 0.0f;
    
    // Average signal strength
    auto activeSignals = getActiveSignals();
    if (!activeSignals.empty()) {
        float totalStrength = 0.0f;
        for (const auto& [token, strength] : activeSignals) {
            totalStrength += strength;
        }
        stats["avg_signal_strength"] = totalStrength / static_cast<float>(activeSignals.size());
    } else {
        stats["avg_signal_strength"] = 0.0f;
    }
    
    return stats;
}

void SearchInterestTracker::updateTokenSignal(const std::string& token, float newStrength) {
    auto now = std::chrono::system_clock::now();
    
    auto it = tokenSignals_.find(token);
    if (it == tokenSignals_.end()) {
        // New token - initialize with new strength
        tokenSignals_[token] = {newStrength, now};
    } else {
        // Existing token - apply EMA smoothing
        auto& [oldStrength, lastUpdate] = it->second;
        
        // Calculate decayed old strength
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate);
        float ageSeconds = static_cast<float>(duration.count());
        float decayFactor = calculateDecayFactor(ageSeconds);
        float decayedOldStrength = oldStrength * decayFactor;
        
        // EMA: new = alpha * new + (1 - alpha) * old
        float smoothedStrength = params_.smoothingAlpha * newStrength +
                                (1.0f - params_.smoothingAlpha) * decayedOldStrength;
        
        tokenSignals_[token] = {smoothedStrength, now};
    }
}

float SearchInterestTracker::calculateDecayFactor(float ageSeconds) const noexcept {
    // Exponential decay: (0.5)^(age / halfLife)
    return std::pow(0.5f, ageSeconds / params_.decayHalfLife);
}

} // namespace audio_config
