#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <memory>
#include <variant>

/**
 * @file common_types.h
 * @brief Common type definitions for AI-driven instrument synthesis system
 * @author AI Synthesis Team
 * @version 1.0
 */

namespace AISynthesis {

/**
 * @brief Range structure for handling min/max or single values
 * 
 * Used throughout the system to represent parameter ranges that can be
 * either a single value or a [min, max] range for AI-driven variation.
 */
struct ParameterRange {
    float minValue{0.0f};
    float maxValue{0.0f};
    
    /**
     * @brief Construct a range from a single value
     * @param value Single value to use for both min and max
     */
    explicit ParameterRange(float value = 0.0f) : minValue(value), maxValue(value) {}
    
    /**
     * @brief Construct a range from min and max values
     * @param min Minimum value
     * @param max Maximum value
     */
    ParameterRange(float min, float max) : minValue(min), maxValue(max) {}
    
    /**
     * @brief Check if this represents a single value (min == max)
     * @return true if single value, false if range
     */
    bool isSingleValue() const noexcept { return minValue == maxValue; }
    
    /**
     * @brief Get a random value within the range
     * @return Random float between min and max (inclusive)
     */
    float getRandomValue() const noexcept;
    
    /**
     * @brief Get the midpoint of the range
     * @return Average of min and max values
     */
    float getMidpoint() const noexcept { return (minValue + maxValue) * 0.5f; }
};

/**
 * @brief Layer roles for composite instrument/effect stacking
 * 
 * Defines the semantic role of each layer in a composite patch,
 * allowing AI to assign appropriate mix levels and processing.
 */
enum class LayerRole {
    BACKGROUND_TEXTURE,  ///< Subtle background elements
    AMBIENT_PAD,         ///< Ambient harmonic content
    SUPPORTIVE_HARMONY,  ///< Supporting harmonic elements
    RHYTHMIC_MOTION,     ///< Rhythmic/percussive elements
    MAIN_MELODIC,        ///< Primary melodic content
    LEAD_FOREGROUND,     ///< Lead/solo foreground elements
    EFFECT_LAYER         ///< Pure effect processing layer
};

/**
 * @brief Convert LayerRole enum to string representation
 * @param role LayerRole enum value
 * @return String representation of the layer role
 */
std::string layerRoleToString(LayerRole role) noexcept;

/**
 * @brief Convert string to LayerRole enum
 * @param roleStr String representation of layer role
 * @return Optional LayerRole if valid, nullopt otherwise
 */
std::optional<LayerRole> stringToLayerRole(const std::string& roleStr) noexcept;

/**
 * @brief Sound characteristics for semantic matching
 * 
 * Contains descriptive characteristics used by AI for semantic
 * keyword matching and scoring. Immutable once constructed.
 */
class SoundCharacteristics {
public:
    /**
     * @brief Construct sound characteristics
     * @param timbralTag Timbral descriptor (warm, bright, etc.)
     * @param materialTag Material descriptor (wood, metal, etc.)
     * @param dynamicTag Dynamic descriptor (punchy, smooth, etc.)
     * @param emotionalTags List of emotional tags with weights
     */
    explicit SoundCharacteristics(
        std::string timbralTag = "",
        std::string materialTag = "",
        std::string dynamicTag = "",
        std::vector<std::pair<std::string, float>> emotionalTags = {}
    );
    
    // Getters (const interface for immutability)
    const std::string& getTimbralTag() const noexcept { return timbralTag_; }
    const std::string& getMaterialTag() const noexcept { return materialTag_; }
    const std::string& getDynamicTag() const noexcept { return dynamicTag_; }
    const std::vector<std::pair<std::string, float>>& getEmotionalTags() const noexcept { return emotionalTags_; }
    
    /**
     * @brief Get all emotional tag names (without weights)
     * @return Vector of emotional tag strings
     */
    std::vector<std::string> getEmotionalTagNames() const;
    
    /**
     * @brief Check if characteristics contain a specific tag
     * @param tag Tag to search for (case-insensitive)
     * @return true if tag found in any category
     */
    bool containsTag(const std::string& tag) const noexcept;

private:
    std::string timbralTag_;
    std::string materialTag_;
    std::string dynamicTag_;
    std::vector<std::pair<std::string, float>> emotionalTags_;
};

/**
 * @brief Topological metadata for AI navigation
 * 
 * Contains metadata used by AI for understanding the position
 * and characteristics of sounds in parameter space.
 */
class TopologicalMetadata {
public:
    /**
     * @brief Construct topological metadata
     * @param dampingLevel Damping characteristic (high, low, medium)
     * @param spectralComplexity Spectral complexity (simple, complex, rich)
     * @param manifoldPosition Position in parameter manifold
     */
    explicit TopologicalMetadata(
        std::string dampingLevel = "",
        std::string spectralComplexity = "",
        std::string manifoldPosition = ""
    );
    
    // Getters
    const std::string& getDampingLevel() const noexcept { return dampingLevel_; }
    const std::string& getSpectralComplexity() const noexcept { return spectralComplexity_; }
    const std::string& getManifoldPosition() const noexcept { return manifoldPosition_; }

private:
    std::string dampingLevel_;
    std::string spectralComplexity_;
    std::string manifoldPosition_;
};

/**
 * @brief Parameter variant type for flexible storage
 * 
 * Allows storage of different parameter types in a type-safe manner
 * while maintaining clear type information for AI processing.
 */
using ParameterValue = std::variant<
    float,
    bool,
    std::string,
    std::vector<float>,
    std::vector<std::string>,
    ParameterRange
>;

/**
 * @brief Parameter metadata for schema validation and UI generation
 * 
 * Contains metadata about parameters for validation, UI generation,
 * and AI understanding of parameter semantics.
 */
struct ParameterMetadata {
    std::string displayName{};        ///< Human-readable parameter name
    std::string parameterType{};      ///< Type identifier (float, bool, string, etc.)
    float minimumValue{0.0f};         ///< Minimum allowed value (for numeric types)
    float maximumValue{1.0f};         ///< Maximum allowed value (for numeric types)
    std::string units{};              ///< Parameter units (Hz, ms, dB, etc.)
    std::string description{};        ///< Detailed parameter description
    bool isRequired{false};           ///< Whether parameter is required
    bool allowsAiControl{false};      ///< Whether AI can modify this parameter
    
    /**
     * @brief Validate a parameter value against this metadata
     * @param value Value to validate
     * @return true if value is valid for this parameter
     */
    bool validateValue(const ParameterValue& value) const noexcept;
    
    /**
     * @brief Clamp a numeric value to the valid range
     * @param value Value to clamp
     * @return Clamped value within [minimumValue, maximumValue]
     */
    float clampValue(float value) const noexcept;
};

/**
 * @brief Configuration quality levels for AI filtering
 * 
 * Used to categorize configurations by quality/completeness
 * for AI suggestion filtering.
 */
enum class ConfigurationQuality {
    REFERENCE_ONLY,    ///< Reference/template only (moods.json, Synthesizer.json)
    DRAFT,            ///< Incomplete configuration
    VALID,            ///< Valid, renderable configuration
    OPTIMIZED         ///< Optimized, high-quality configuration
};

/**
 * @brief Instrument types supported by the system
 */
enum class InstrumentType {
    GUITAR_ACOUSTIC,
    GUITAR_ELECTRIC,
    SYNTHESIZER_SUBTRACTIVE,
    SYNTHESIZER_FM,
    SYNTHESIZER_ADDITIVE,
    SYNTHESIZER_WAVETABLE,
    DRUMS_ACOUSTIC,
    DRUMS_ELECTRONIC,
    BASS_ELECTRIC,
    BASS_SYNTHESIZED,
    UNKNOWN
};

/**
 * @brief Convert InstrumentType to string
 * @param type InstrumentType enum value
 * @return String representation
 */
std::string instrumentTypeToString(InstrumentType type) noexcept;

/**
 * @brief Convert string to InstrumentType
 * @param typeStr String representation
 * @return InstrumentType if valid, UNKNOWN otherwise
 */
InstrumentType stringToInstrumentType(const std::string& typeStr) noexcept;

/**
 * @brief Type-safe identifier for configurations
 * 
 * Prevents mixing up different types of configuration IDs
 */
template<typename Tag>
class TypedId {
public:
    explicit TypedId(std::string id) : id_(std::move(id)) {}
    
    const std::string& getValue() const noexcept { return id_; }
    
    bool operator==(const TypedId& other) const noexcept { return id_ == other.id_; }
    bool operator!=(const TypedId& other) const noexcept { return id_ != other.id_; }
    bool operator<(const TypedId& other) const noexcept { return id_ < other.id_; }

private:
    std::string id_;
};

// Typed ID tags for different configuration types
struct ConfigurationIdTag {};
struct LayerIdTag {};
struct EffectIdTag {};

using ConfigurationId = TypedId<ConfigurationIdTag>;
using LayerId = TypedId<LayerIdTag>;
using EffectId = TypedId<EffectIdTag>;

} // namespace AISynthesis