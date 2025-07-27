#pragma once

#include "common_types.h"
#include <nlohmann/json.hpp>
#include <map>
#include <string>
#include <vector>
#include <memory>

/**
 * @file audio_config.h
 * @brief Audio configuration classes for instrument and effect parameter management
 * @author AI Synthesis Team
 * @version 1.0
 */

namespace AISynthesis {

/**
 * @brief Base class for parameter storage with type safety and validation
 * 
 * Provides a foundation for storing, validating, and managing audio parameters
 * with schema support and type-safe access methods. Follows RAII principles.
 */
class BaseParameterContainer {
public:
    /**
     * @brief Construct empty parameter container
     */
    explicit BaseParameterContainer() = default;
    
    /**
     * @brief Virtual destructor for proper inheritance
     */
    virtual ~BaseParameterContainer() = default;
    
    // Delete copy constructor and assignment (use move semantics)
    BaseParameterContainer(const BaseParameterContainer&) = delete;
    BaseParameterContainer& operator=(const BaseParameterContainer&) = delete;
    
    // Enable move semantics
    BaseParameterContainer(BaseParameterContainer&&) = default;
    BaseParameterContainer& operator=(BaseParameterContainer&&) = default;
    
    /**
     * @brief Store a parameter with type checking and validation
     * @param key Parameter name
     * @param value Parameter value
     * @param context Context string for error reporting
     * @return true if parameter was stored successfully
     */
    bool storeParameter(const std::string& key, const ParameterValue& value, 
                       const std::string& context = "");
    
    /**
     * @brief Get a float parameter with default value
     * @param key Parameter name
     * @param defaultValue Default value if parameter not found
     * @return Parameter value or default
     */
    float getFloatParameter(const std::string& key, float defaultValue = 0.0f) const noexcept;
    
    /**
     * @brief Get a bool parameter with default value
     * @param key Parameter name
     * @param defaultValue Default value if parameter not found
     * @return Parameter value or default
     */
    bool getBoolParameter(const std::string& key, bool defaultValue = false) const noexcept;
    
    /**
     * @brief Get a string parameter with default value
     * @param key Parameter name
     * @param defaultValue Default value if parameter not found
     * @return Parameter value or default
     */
    std::string getStringParameter(const std::string& key, 
                                  const std::string& defaultValue = "") const noexcept;
    
    /**
     * @brief Get a vector parameter with default value
     * @param key Parameter name
     * @param defaultValue Default value if parameter not found
     * @return Parameter value or default
     */
    std::vector<float> getVectorParameter(const std::string& key, 
                                         const std::vector<float>& defaultValue = {}) const noexcept;
    
    /**
     * @brief Get all parameter keys
     * @return Vector of all stored parameter keys
     */
    std::vector<std::string> getAllParameterKeys() const noexcept;
    
    /**
     * @brief Check if parameter exists
     * @param key Parameter name
     * @return true if parameter exists
     */
    bool hasParameter(const std::string& key) const noexcept;
    
    /**
     * @brief Clear all parameters
     */
    void clearParameters() noexcept;
    
    /**
     * @brief Load parameters from JSON with validation
     * @param jsonData JSON object containing parameters
     * @param context Context string for error reporting
     */
    virtual void loadFromJson(const nlohmann::json& jsonData, const std::string& context = "");
    
    /**
     * @brief Export parameters to JSON
     * @return JSON object containing all parameters
     */
    virtual nlohmann::json exportToJson() const;
    
    /**
     * @brief Register parameter schema for validation
     * @param key Parameter name
     * @param metadata Parameter metadata for validation
     */
    void registerParameterSchema(const std::string& key, const ParameterMetadata& metadata);
    
    /**
     * @brief Validate all parameters against schema
     * @return true if all parameters are valid
     */
    bool validateParameters() const noexcept;

protected:
    std::map<std::string, ParameterValue> parameters_;
    std::map<std::string, ParameterMetadata> parameterSchema_;
    
    /**
     * @brief Validate a single parameter value
     * @param key Parameter name
     * @param value Parameter value
     * @return true if parameter is valid
     */
    bool validateParameterValue(const std::string& key, const ParameterValue& value) const noexcept;
    
    /**
     * @brief Log validation error
     * @param message Error message
     * @param context Context string
     */
    void logValidationError(const std::string& message, const std::string& context) const noexcept;
};

/**
 * @brief Oscillator configuration with type-safe parameter management
 * 
 * Manages oscillator parameters including waveform types, modulation,
 * and synthesis-specific settings with proper validation.
 */
class OscillatorConfig : public BaseParameterContainer {
public:
    /**
     * @brief Construct oscillator configuration
     */
    explicit OscillatorConfig();
    
    /**
     * @brief Get oscillator waveform types
     * @return Vector of waveform type strings
     */
    std::vector<std::string> getWaveformTypes() const noexcept;
    
    /**
     * @brief Set oscillator waveform types
     * @param types Vector of waveform type strings
     */
    void setWaveformTypes(const std::vector<std::string>& types);
    
    /**
     * @brief Get mix ratios for multiple oscillators
     * @return Vector of mix ratio values
     */
    std::vector<float> getMixRatios() const noexcept;
    
    /**
     * @brief Set mix ratios for multiple oscillators
     * @param ratios Vector of mix ratio values (must sum to 1.0)
     */
    void setMixRatios(const std::vector<float>& ratios);
    
    /**
     * @brief Get detune amount in cents
     * @return Detune value in cents
     */
    float getDetuneCents() const noexcept;
    
    /**
     * @brief Set detune amount in cents
     * @param cents Detune value in cents
     */
    void setDetuneCents(float cents);
    
    /**
     * @brief Load oscillator configuration from JSON
     * @param jsonData JSON object containing oscillator data
     * @param context Context string for error reporting
     */
    void loadFromJson(const nlohmann::json& jsonData, const std::string& context = "") override;

private:
    void initializeSchema();
};

/**
 * @brief Envelope configuration with ADSR and extended envelope support
 * 
 * Manages envelope parameters with support for different envelope types
 * (ADSR, AHDSR, etc.) and parameter validation.
 */
class EnvelopeConfig : public BaseParameterContainer {
public:
    /**
     * @brief Envelope types supported by the system
     */
    enum class EnvelopeType {
        ADSR,      ///< Attack, Decay, Sustain, Release
        AHDSR,     ///< Attack, Hold, Decay, Sustain, Release
        ADHSR,     ///< Attack, Decay, Hold, Sustain, Release
        COMPLEX    ///< Multi-stage envelope
    };
    
    /**
     * @brief Construct envelope configuration
     * @param type Envelope type (default: ADSR)
     */
    explicit EnvelopeConfig(EnvelopeType type = EnvelopeType::ADSR);
    
    /**
     * @brief Get envelope type
     * @return Current envelope type
     */
    EnvelopeType getEnvelopeType() const noexcept { return envelopeType_; }
    
    /**
     * @brief Set envelope type
     * @param type New envelope type
     */
    void setEnvelopeType(EnvelopeType type);
    
    /**
     * @brief Get attack time in seconds
     * @return Attack time
     */
    float getAttackTime() const noexcept;
    
    /**
     * @brief Set attack time in seconds
     * @param attackTime Attack time (must be >= 0)
     */
    void setAttackTime(float attackTime);
    
    /**
     * @brief Get decay time in seconds
     * @return Decay time
     */
    float getDecayTime() const noexcept;
    
    /**
     * @brief Set decay time in seconds
     * @param decayTime Decay time (must be >= 0)
     */
    void setDecayTime(float decayTime);
    
    /**
     * @brief Get sustain level (0-1)
     * @return Sustain level
     */
    float getSustainLevel() const noexcept;
    
    /**
     * @brief Set sustain level (0-1)
     * @param sustainLevel Sustain level (clamped to 0-1)
     */
    void setSustainLevel(float sustainLevel);
    
    /**
     * @brief Get release time in seconds
     * @return Release time
     */
    float getReleaseTime() const noexcept;
    
    /**
     * @brief Set release time in seconds
     * @param releaseTime Release time (must be >= 0)
     */
    void setReleaseTime(float releaseTime);
    
    /**
     * @brief Load envelope configuration from JSON
     * @param jsonData JSON object containing envelope data
     * @param context Context string for error reporting
     */
    void loadFromJson(const nlohmann::json& jsonData, const std::string& context = "") override;

private:
    EnvelopeType envelopeType_;
    
    void initializeSchema();
    void validateEnvelopeParameters();
};

/**
 * @brief Filter configuration with multiple filter types and modulation
 * 
 * Manages filter parameters including type, cutoff, resonance, and
 * envelope modulation with proper validation and type safety.
 */
class FilterConfig : public BaseParameterContainer {
public:
    /**
     * @brief Filter types supported by the system
     */
    enum class FilterType {
        LOW_PASS,
        HIGH_PASS,
        BAND_PASS,
        BAND_REJECT,
        NOTCH,
        ALL_PASS,
        LOW_SHELF,
        HIGH_SHELF,
        PEAK
    };
    
    /**
     * @brief Construct filter configuration
     * @param type Filter type (default: LOW_PASS)
     */
    explicit FilterConfig(FilterType type = FilterType::LOW_PASS);
    
    /**
     * @brief Get filter type
     * @return Current filter type
     */
    FilterType getFilterType() const noexcept { return filterType_; }
    
    /**
     * @brief Set filter type
     * @param type New filter type
     */
    void setFilterType(FilterType type);
    
    /**
     * @brief Get cutoff frequency in Hz
     * @return Cutoff frequency
     */
    float getCutoffFrequency() const noexcept;
    
    /**
     * @brief Set cutoff frequency in Hz
     * @param frequency Cutoff frequency (must be > 0)
     */
    void setCutoffFrequency(float frequency);
    
    /**
     * @brief Get resonance value (Q factor)
     * @return Resonance value
     */
    float getResonance() const noexcept;
    
    /**
     * @brief Set resonance value (Q factor)
     * @param resonance Resonance value (must be > 0)
     */
    void setResonance(float resonance);
    
    /**
     * @brief Get envelope modulation amount
     * @return Envelope modulation amount (-1 to 1)
     */
    float getEnvelopeAmount() const noexcept;
    
    /**
     * @brief Set envelope modulation amount
     * @param amount Envelope modulation amount (-1 to 1)
     */
    void setEnvelopeAmount(float amount);
    
    /**
     * @brief Load filter configuration from JSON
     * @param jsonData JSON object containing filter data
     * @param context Context string for error reporting
     */
    void loadFromJson(const nlohmann::json& jsonData, const std::string& context = "") override;

private:
    FilterType filterType_;
    
    void initializeSchema();
    static FilterType stringToFilterType(const std::string& typeString) noexcept;
    static std::string filterTypeToString(FilterType type) noexcept;
};

/**
 * @brief Effect configuration with plugin-style parameter management
 * 
 * Manages effect parameters with support for different effect types,
 * parameter mapping, and AI-controlled modulation settings.
 */
class EffectConfig : public BaseParameterContainer {
public:
    /**
     * @brief Construct effect configuration
     * @param effectType Type of effect (reverb, delay, distortion, etc.)
     */
    explicit EffectConfig(std::string effectType = "");
    
    /**
     * @brief Get effect type
     * @return Effect type string
     */
    const std::string& getEffectType() const noexcept { return effectType_; }
    
    /**
     * @brief Set effect type
     * @param type New effect type
     */
    void setEffectType(const std::string& type);
    
    /**
     * @brief Get wet/dry mix (0 = fully dry, 1 = fully wet)
     * @return Mix value
     */
    float getMixLevel() const noexcept;
    
    /**
     * @brief Set wet/dry mix level
     * @param mix Mix value (clamped to 0-1)
     */
    void setMixLevel(float mix);
    
    /**
     * @brief Check if AI control is enabled for this effect
     * @return true if AI can control this effect
     */
    bool isAiControlEnabled() const noexcept;
    
    /**
     * @brief Enable/disable AI control for this effect
     * @param enabled true to enable AI control
     */
    void setAiControlEnabled(bool enabled);
    
    /**
     * @brief Load effect configuration from JSON
     * @param jsonData JSON object containing effect data
     * @param context Context string for error reporting
     */
    void loadFromJson(const nlohmann::json& jsonData, const std::string& context = "") override;
    
    /**
     * @brief Register schema for specific effect type
     * @param effectType Effect type identifier
     * @param schema Map of parameter names to metadata
     */
    static void registerEffectSchema(const std::string& effectType, 
                                   const std::map<std::string, ParameterMetadata>& schema);

private:
    std::string effectType_;
    static std::map<std::string, std::map<std::string, ParameterMetadata>> effectSchemas_;
    
    void initializeSchema();
    void loadEffectSpecificSchema();
};

/**
 * @brief Complete instrument configuration container
 * 
 * Aggregates all configuration components (oscillator, envelope, filter, effects)
 * with sound characteristics and topological metadata for AI processing.
 */
class InstrumentConfig {
public:
    /**
     * @brief Construct instrument configuration
     * @param configId Unique identifier for this configuration
     * @param instrumentType Type of instrument
     */
    explicit InstrumentConfig(ConfigurationId configId, InstrumentType instrumentType);
    
    /**
     * @brief Virtual destructor for proper inheritance
     */
    virtual ~InstrumentConfig() = default;
    
    // Delete copy, enable move
    InstrumentConfig(const InstrumentConfig&) = delete;
    InstrumentConfig& operator=(const InstrumentConfig&) = delete;
    InstrumentConfig(InstrumentConfig&&) = default;
    InstrumentConfig& operator=(InstrumentConfig&&) = default;
    
    /**
     * @brief Get configuration ID
     * @return Configuration identifier
     */
    const ConfigurationId& getConfigurationId() const noexcept { return configId_; }
    
    /**
     * @brief Get instrument type
     * @return Instrument type
     */
    InstrumentType getInstrumentType() const noexcept { return instrumentType_; }
    
    /**
     * @brief Get oscillator configuration
     * @return Reference to oscillator config
     */
    OscillatorConfig& getOscillatorConfig() noexcept { return oscillatorConfig_; }
    const OscillatorConfig& getOscillatorConfig() const noexcept { return oscillatorConfig_; }
    
    /**
     * @brief Get envelope configuration
     * @return Reference to envelope config
     */
    EnvelopeConfig& getEnvelopeConfig() noexcept { return envelopeConfig_; }
    const EnvelopeConfig& getEnvelopeConfig() const noexcept { return envelopeConfig_; }
    
    /**
     * @brief Get filter configuration
     * @return Reference to filter config
     */
    FilterConfig& getFilterConfig() noexcept { return filterConfig_; }
    const FilterConfig& getFilterConfig() const noexcept { return filterConfig_; }
    
    /**
     * @brief Get all effect configurations
     * @return Vector of effect configurations
     */
    const std::vector<std::unique_ptr<EffectConfig>>& getEffectConfigs() const noexcept { return effectConfigs_; }
    
    /**
     * @brief Add effect configuration
     * @param effectConfig Unique pointer to effect configuration
     */
    void addEffectConfig(std::unique_ptr<EffectConfig> effectConfig);
    
    /**
     * @brief Remove effect configuration by index
     * @param index Index of effect to remove
     */
    void removeEffectConfig(std::size_t index);
    
    /**
     * @brief Get sound characteristics
     * @return Reference to sound characteristics
     */
    const SoundCharacteristics& getSoundCharacteristics() const noexcept { return soundCharacteristics_; }
    
    /**
     * @brief Set sound characteristics
     * @param characteristics New sound characteristics
     */
    void setSoundCharacteristics(SoundCharacteristics characteristics);
    
    /**
     * @brief Get topological metadata
     * @return Reference to topological metadata
     */
    const TopologicalMetadata& getTopologicalMetadata() const noexcept { return topologicalMetadata_; }
    
    /**
     * @brief Set topological metadata
     * @param metadata New topological metadata
     */
    void setTopologicalMetadata(TopologicalMetadata metadata);
    
    /**
     * @brief Get configuration quality level
     * @return Configuration quality
     */
    ConfigurationQuality getQuality() const noexcept { return quality_; }
    
    /**
     * @brief Set configuration quality level
     * @param quality New quality level
     */
    void setQuality(ConfigurationQuality quality) { quality_ = quality; }
    
    /**
     * @brief Load complete configuration from JSON
     * @param jsonData JSON object containing configuration data
     * @param context Context string for error reporting
     */
    virtual void loadFromJson(const nlohmann::json& jsonData, const std::string& context = "");
    
    /**
     * @brief Export complete configuration to JSON
     * @return JSON object containing all configuration data
     */
    virtual nlohmann::json exportToJson() const;
    
    /**
     * @brief Validate complete configuration
     * @return true if configuration is valid and complete
     */
    bool validateConfiguration() const noexcept;

protected:
    ConfigurationId configId_;
    InstrumentType instrumentType_;
    ConfigurationQuality quality_{ConfigurationQuality::DRAFT};
    
    OscillatorConfig oscillatorConfig_;
    EnvelopeConfig envelopeConfig_;
    FilterConfig filterConfig_;
    std::vector<std::unique_ptr<EffectConfig>> effectConfigs_;
    
    SoundCharacteristics soundCharacteristics_;
    TopologicalMetadata topologicalMetadata_;
    
    /**
     * @brief Perform instrument-specific validation
     * @return true if instrument-specific parameters are valid
     */
    virtual bool validateInstrumentSpecific() const noexcept;
};

} // namespace AISynthesis