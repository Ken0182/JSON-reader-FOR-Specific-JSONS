#pragma once

#include "common_types.h"
#include "audio_config.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <memory>

/**
 * @file json_parser.h
 * @brief Type-safe JSON parsing utilities for audio configurations
 * @author AI Synthesis Team
 * @version 1.0
 */

namespace AISynthesis {

/**
 * @brief JSON parsing error information
 */
struct JsonParsingError {
    std::string message{};          ///< Error description
    std::string context{};          ///< Context where error occurred
    std::string fieldPath{};        ///< JSON field path (e.g., "config.envelope.attack")
    int lineNumber{-1};             ///< Line number in JSON file (if available)
    
    /**
     * @brief Get formatted error message
     * @return Human-readable error description
     */
    std::string getFormattedMessage() const noexcept;
};

/**
 * @brief JSON parsing result with error handling
 * 
 * Template class for type-safe JSON parsing results that either
 * contain a successfully parsed object or detailed error information.
 */
template<typename T>
class JsonParsingResult {
public:
    /**
     * @brief Construct successful result
     * @param value Successfully parsed value
     */
    explicit JsonParsingResult(T value) : value_(std::move(value)), hasError_(false) {}
    
    /**
     * @brief Construct error result
     * @param error Error information
     */
    explicit JsonParsingResult(JsonParsingError error) : error_(std::move(error)), hasError_(true) {}
    
    /**
     * @brief Check if parsing was successful
     * @return true if value is available, false if error occurred
     */
    bool isSuccess() const noexcept { return !hasError_; }
    
    /**
     * @brief Check if parsing failed
     * @return true if error occurred, false if value is available
     */
    bool hasError() const noexcept { return hasError_; }
    
    /**
     * @brief Get parsed value (only if successful)
     * @return Reference to parsed value
     * @throws std::runtime_error if called on error result
     */
    const T& getValue() const {
        if (hasError_) {
            throw std::runtime_error("Attempted to get value from error result: " + error_.getFormattedMessage());
        }
        return value_;
    }
    
    /**
     * @brief Get error information (only if failed)
     * @return Reference to error information
     * @throws std::runtime_error if called on success result
     */
    const JsonParsingError& getError() const {
        if (!hasError_) {
            throw std::runtime_error("Attempted to get error from success result");
        }
        return error_;
    }
    
    /**
     * @brief Get value or default if error occurred
     * @param defaultValue Default value to return on error
     * @return Parsed value or default
     */
    T getValueOrDefault(const T& defaultValue) const noexcept {
        return hasError_ ? defaultValue : value_;
    }

private:
    T value_{};
    JsonParsingError error_{};
    bool hasError_;
};

/**
 * @brief Type-safe JSON value parser with schema validation
 * 
 * Provides utilities for parsing JSON values with proper type checking,
 * range validation, and detailed error reporting. Follows defensive
 * programming principles.
 */
class JsonValueParser {
public:
    /**
     * @brief Parse a float value with optional range validation
     * @param jsonValue JSON value to parse
     * @param context Context string for error reporting
     * @param minValue Minimum allowed value (optional)
     * @param maxValue Maximum allowed value (optional)
     * @return Parsing result with float value or error
     */
    static JsonParsingResult<float> parseFloat(
        const nlohmann::json& jsonValue,
        const std::string& context = "",
        std::optional<float> minValue = std::nullopt,
        std::optional<float> maxValue = std::nullopt
    ) noexcept;
    
    /**
     * @brief Parse a float value with unit conversion support
     * @param jsonValue JSON value to parse (supports "100ms", "2.5s", "440Hz", etc.)
     * @param context Context string for error reporting
     * @param targetUnit Target unit for conversion ("s", "ms", "Hz")
     * @return Parsing result with converted float value or error
     */
    static JsonParsingResult<float> parseFloatWithUnits(
        const nlohmann::json& jsonValue,
        const std::string& context = "",
        const std::string& targetUnit = ""
    ) noexcept;
    
    /**
     * @brief Parse a boolean value
     * @param jsonValue JSON value to parse
     * @param context Context string for error reporting
     * @return Parsing result with boolean value or error
     */
    static JsonParsingResult<bool> parseBool(
        const nlohmann::json& jsonValue,
        const std::string& context = ""
    ) noexcept;
    
    /**
     * @brief Parse a string value with optional validation
     * @param jsonValue JSON value to parse
     * @param context Context string for error reporting
     * @param allowedValues Optional list of allowed string values
     * @return Parsing result with string value or error
     */
    static JsonParsingResult<std::string> parseString(
        const nlohmann::json& jsonValue,
        const std::string& context = "",
        const std::vector<std::string>& allowedValues = {}
    ) noexcept;
    
    /**
     * @brief Parse a vector of floats
     * @param jsonValue JSON array to parse
     * @param context Context string for error reporting
     * @param minSize Minimum required array size (optional)
     * @param maxSize Maximum allowed array size (optional)
     * @return Parsing result with float vector or error
     */
    static JsonParsingResult<std::vector<float>> parseFloatVector(
        const nlohmann::json& jsonValue,
        const std::string& context = "",
        std::optional<std::size_t> minSize = std::nullopt,
        std::optional<std::size_t> maxSize = std::nullopt
    ) noexcept;
    
    /**
     * @brief Parse a vector of strings
     * @param jsonValue JSON array to parse
     * @param context Context string for error reporting
     * @param allowedValues Optional list of allowed string values
     * @return Parsing result with string vector or error
     */
    static JsonParsingResult<std::vector<std::string>> parseStringVector(
        const nlohmann::json& jsonValue,
        const std::string& context = "",
        const std::vector<std::string>& allowedValues = {}
    ) noexcept;
    
    /**
     * @brief Parse a parameter range (single value or [min, max] array)
     * @param jsonValue JSON value or array to parse
     * @param context Context string for error reporting
     * @return Parsing result with ParameterRange or error
     */
    static JsonParsingResult<ParameterRange> parseParameterRange(
        const nlohmann::json& jsonValue,
        const std::string& context = ""
    ) noexcept;
    
    /**
     * @brief Validate JSON object structure against expected fields
     * @param jsonObject JSON object to validate
     * @param requiredFields List of required field names
     * @param optionalFields List of optional field names
     * @param context Context string for error reporting
     * @return Optional error if validation fails, nullopt if valid
     */
    static std::optional<JsonParsingError> validateObjectStructure(
        const nlohmann::json& jsonObject,
        const std::vector<std::string>& requiredFields = {},
        const std::vector<std::string>& optionalFields = {},
        const std::string& context = ""
    ) noexcept;
    
    /**
     * @brief Check if JSON value is of expected type
     * @param jsonValue JSON value to check
     * @param expectedType Expected JSON type
     * @param context Context string for error reporting
     * @return Optional error if type mismatch, nullopt if correct
     */
    static std::optional<JsonParsingError> validateJsonType(
        const nlohmann::json& jsonValue,
        nlohmann::json::value_t expectedType,
        const std::string& context = ""
    ) noexcept;

private:
    /**
     * @brief Convert time value to seconds
     * @param value Numeric value
     * @param unit Unit string ("ms", "s")
     * @return Value converted to seconds
     */
    static float convertTimeToSeconds(float value, const std::string& unit) noexcept;
    
    /**
     * @brief Extract unit from string value
     * @param stringValue String containing number and unit
     * @return Pair of (numeric_value, unit_string)
     */
    static std::pair<float, std::string> extractValueAndUnit(const std::string& stringValue) noexcept;
    
    /**
     * @brief Create JSON parsing error
     * @param message Error message
     * @param context Context string
     * @return JsonParsingError object
     */
    static JsonParsingError createError(const std::string& message, const std::string& context) noexcept;
};

/**
 * @brief Configuration file loader with schema validation
 * 
 * Handles loading and parsing of complete configuration files with
 * proper error handling, validation, and type safety.
 */
class ConfigurationFileLoader {
public:
    /**
     * @brief File loading result
     */
    struct LoadingResult {
        std::vector<std::unique_ptr<InstrumentConfig>> configurations{};   ///< Successfully loaded configurations
        std::vector<JsonParsingError> errors{};                           ///< Parsing errors encountered
        std::vector<JsonParsingError> warnings{};                         ///< Non-fatal warnings
        bool hasErrors{false};                                             ///< Whether any errors occurred
        std::string sourceFile{};                                          ///< Source file path
    };
    
    /**
     * @brief Construct configuration file loader
     */
    explicit ConfigurationFileLoader() = default;
    
    /**
     * @brief Load configurations from JSON file
     * @param filePath Path to JSON configuration file
     * @param validateSchema Whether to perform schema validation
     * @return Loading result with configurations and errors
     */
    LoadingResult loadFromFile(const std::string& filePath, bool validateSchema = true) noexcept;
    
    /**
     * @brief Load configurations from JSON data
     * @param jsonData JSON object containing configuration data
     * @param context Context string for error reporting
     * @param validateSchema Whether to perform schema validation
     * @return Loading result with configurations and errors
     */
    LoadingResult loadFromJson(
        const nlohmann::json& jsonData,
        const std::string& context = "",
        bool validateSchema = true
    ) noexcept;
    
    /**
     * @brief Load guitar configurations specifically
     * @param filePath Path to guitar.json file
     * @return Loading result with guitar configurations
     */
    LoadingResult loadGuitarConfigurations(const std::string& filePath) noexcept;
    
    /**
     * @brief Load group/synthesizer configurations specifically
     * @param filePath Path to group.json file
     * @return Loading result with group configurations
     */
    LoadingResult loadGroupConfigurations(const std::string& filePath) noexcept;
    
    /**
     * @brief Load reference configurations (moods.json, Synthesizer.json)
     * @param filePath Path to reference JSON file
     * @return Loading result with reference data (marked as REFERENCE_ONLY quality)
     */
    LoadingResult loadReferenceConfigurations(const std::string& filePath) noexcept;
    
    /**
     * @brief Validate configuration file format
     * @param filePath Path to JSON file
     * @return Vector of validation errors (empty if valid)
     */
    std::vector<JsonParsingError> validateConfigurationFile(const std::string& filePath) noexcept;

private:
    /**
     * @brief Parse single instrument configuration
     * @param configJson JSON object for one configuration
     * @param configId Configuration identifier
     * @param context Context string for error reporting
     * @return Parsed instrument configuration or error
     */
    JsonParsingResult<std::unique_ptr<InstrumentConfig>> parseInstrumentConfig(
        const nlohmann::json& configJson,
        const ConfigurationId& configId,
        const std::string& context
    ) noexcept;
    
    /**
     * @brief Parse oscillator section from JSON
     * @param oscillatorJson JSON object containing oscillator data
     * @param context Context string for error reporting
     * @return Parsed oscillator configuration or error
     */
    JsonParsingResult<OscillatorConfig> parseOscillatorConfig(
        const nlohmann::json& oscillatorJson,
        const std::string& context
    ) noexcept;
    
    /**
     * @brief Parse envelope section from JSON
     * @param envelopeJson JSON object containing envelope data
     * @param context Context string for error reporting
     * @return Parsed envelope configuration or error
     */
    JsonParsingResult<EnvelopeConfig> parseEnvelopeConfig(
        const nlohmann::json& envelopeJson,
        const std::string& context
    ) noexcept;
    
    /**
     * @brief Parse filter section from JSON
     * @param filterJson JSON object containing filter data
     * @param context Context string for error reporting
     * @return Parsed filter configuration or error
     */
    JsonParsingResult<FilterConfig> parseFilterConfig(
        const nlohmann::json& filterJson,
        const std::string& context
    ) noexcept;
    
    /**
     * @brief Parse effects array from JSON
     * @param effectsJson JSON array containing effect data
     * @param context Context string for error reporting
     * @return Parsed effect configurations or error
     */
    JsonParsingResult<std::vector<std::unique_ptr<EffectConfig>>> parseEffectConfigs(
        const nlohmann::json& effectsJson,
        const std::string& context
    ) noexcept;
    
    /**
     * @brief Parse sound characteristics from JSON
     * @param characteristicsJson JSON object containing sound characteristics
     * @param context Context string for error reporting
     * @return Parsed sound characteristics or error
     */
    JsonParsingResult<SoundCharacteristics> parseSoundCharacteristics(
        const nlohmann::json& characteristicsJson,
        const std::string& context
    ) noexcept;
    
    /**
     * @brief Parse topological metadata from JSON
     * @param metadataJson JSON object containing topological metadata
     * @param context Context string for error reporting
     * @return Parsed topological metadata or error
     */
    JsonParsingResult<TopologicalMetadata> parseTopologicalMetadata(
        const nlohmann::json& metadataJson,
        const std::string& context
    ) noexcept;
    
    /**
     * @brief Infer instrument type from configuration content
     * @param configJson JSON configuration object
     * @param configId Configuration identifier
     * @return Inferred instrument type
     */
    InstrumentType inferInstrumentType(
        const nlohmann::json& configJson,
        const ConfigurationId& configId
    ) noexcept;
    
    /**
     * @brief Resolve field aliases in JSON object
     * @param jsonObject JSON object to modify
     * @param context Context string for logging
     */
    void resolveFieldAliases(nlohmann::json& jsonObject, const std::string& context) noexcept;
    
    /**
     * @brief Validate against expected schema
     * @param jsonObject JSON object to validate
     * @param configType Configuration type for schema selection
     * @param context Context string for error reporting
     * @return Vector of validation errors
     */
    std::vector<JsonParsingError> validateAgainstSchema(
        const nlohmann::json& jsonObject,
        const std::string& configType,
        const std::string& context
    ) noexcept;
};

/**
 * @brief Configuration file writer for exporting configurations
 * 
 * Handles writing configurations to JSON files with proper formatting,
 * validation, and organization. Ensures only valid, renderable
 * configurations are written to output files.
 */
class ConfigurationFileWriter {
public:
    /**
     * @brief Writing options
     */
    struct WritingOptions {
        bool prettyPrint{true};             ///< Format JSON with indentation
        bool includeMetadata{true};         ///< Include metadata sections
        bool includeSchema{false};          ///< Include schema information
        bool compressEmptyValues{true};     ///< Remove null/empty values
        bool validateBeforeWrite{true};     ///< Validate configurations before writing
        int indentSize{4};                  ///< Number of spaces for indentation
    };
    
    /**
     * @brief Writing result
     */
    struct WritingResult {
        bool success{false};                        ///< Whether write was successful
        std::vector<JsonParsingError> errors{};     ///< Errors encountered during writing
        std::vector<JsonParsingError> warnings{};   ///< Non-fatal warnings
        std::string outputFile{};                   ///< Output file path
    };
    
    /**
     * @brief Construct configuration file writer
     * @param options Writing options to use
     */
    explicit ConfigurationFileWriter(WritingOptions options = {});
    
    /**
     * @brief Write configurations to file
     * @param configurations Map of configurations to write
     * @param filePath Output file path
     * @return Writing result with success status and errors
     */
    WritingResult writeToFile(
        const std::map<ConfigurationId, std::shared_ptr<const InstrumentConfig>>& configurations,
        const std::string& filePath
    ) noexcept;
    
    /**
     * @brief Write configurations grouped by instrument type
     * @param configurations Map of configurations to write
     * @param outputDirectory Directory for output files
     * @param filenamePrefix Optional prefix for output files
     * @return Map of instrument type to writing result
     */
    std::map<InstrumentType, WritingResult> writeGroupedFiles(
        const std::map<ConfigurationId, std::shared_ptr<const InstrumentConfig>>& configurations,
        const std::string& outputDirectory,
        const std::string& filenamePrefix = ""
    ) noexcept;
    
    /**
     * @brief Convert configurations to JSON
     * @param configurations Map of configurations to convert
     * @return JSON object containing all configurations
     */
    nlohmann::json convertToJson(
        const std::map<ConfigurationId, std::shared_ptr<const InstrumentConfig>>& configurations
    ) const noexcept;
    
    /**
     * @brief Set writing options
     * @param options New writing options
     */
    void setWritingOptions(const WritingOptions& options) noexcept { options_ = options; }
    
    /**
     * @brief Get current writing options
     * @return Current writing options
     */
    const WritingOptions& getWritingOptions() const noexcept { return options_; }

private:
    WritingOptions options_;
    
    /**
     * @brief Filter configurations for writing (only valid ones)
     * @param configurations Input configurations
     * @return Map of valid configurations
     */
    std::map<ConfigurationId, std::shared_ptr<const InstrumentConfig>> filterValidConfigurations(
        const std::map<ConfigurationId, std::shared_ptr<const InstrumentConfig>>& configurations
    ) const noexcept;
    
    /**
     * @brief Clean JSON output according to options
     * @param jsonData JSON to clean
     * @return Cleaned JSON object
     */
    nlohmann::json cleanJsonOutput(const nlohmann::json& jsonData) const noexcept;
    
    /**
     * @brief Generate filename for instrument type
     * @param instrumentType Instrument type
     * @param prefix Optional filename prefix
     * @return Generated filename
     */
    std::string generateFilename(InstrumentType instrumentType, const std::string& prefix = "") const noexcept;
};

} // namespace AISynthesis