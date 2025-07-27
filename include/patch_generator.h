#pragma once

#include "common_types.h"
#include "audio_config.h"
#include "ai_scorer.h"
#include <nlohmann/json.hpp>
#include <memory>
#include <map>
#include <vector>
#include <string>

/**
 * @file patch_generator.h
 * @brief Layered patch generation and composite configuration management
 * @author AI Synthesis Team
 * @version 1.0
 */

namespace AISynthesis {

/**
 * @brief Layer configuration with role and mix parameters
 * 
 * Represents a single layer in a composite patch with its assigned role,
 * gain settings, and configuration reference. Follows immutable design
 * once constructed.
 */
class LayerConfiguration {
public:
    /**
     * @brief Construct layer configuration
     * @param layerId Unique identifier for this layer
     * @param role Semantic role of this layer
     * @param instrumentConfig Reference to the instrument configuration
     * @param gain Layer gain/mix level (0.0 to 1.0)
     */
    explicit LayerConfiguration(
        LayerId layerId,
        LayerRole role,
        std::shared_ptr<const InstrumentConfig> instrumentConfig,
        float gain = 0.7f
    );
    
    /**
     * @brief Get layer identifier
     * @return Layer ID
     */
    const LayerId& getLayerId() const noexcept { return layerId_; }
    
    /**
     * @brief Get layer role
     * @return Layer role enum
     */
    LayerRole getLayerRole() const noexcept { return layerRole_; }
    
    /**
     * @brief Get layer gain level
     * @return Gain value (0.0 to 1.0)
     */
    float getGainLevel() const noexcept { return gainLevel_; }
    
    /**
     * @brief Get instrument configuration
     * @return Shared pointer to instrument configuration
     */
    std::shared_ptr<const InstrumentConfig> getInstrumentConfig() const noexcept { return instrumentConfig_; }
    
    /**
     * @brief Check if layer is enabled
     * @return true if layer is active
     */
    bool isEnabled() const noexcept { return isEnabled_; }
    
    /**
     * @brief Get pan position (-1.0 left to 1.0 right)
     * @return Pan position
     */
    float getPanPosition() const noexcept { return panPosition_; }
    
    /**
     * @brief Get layer priority (higher = more important)
     * @return Priority value
     */
    int getPriority() const noexcept { return priority_; }
    
    /**
     * @brief Create a modified copy with new gain level
     * @param newGain New gain level (0.0 to 1.0)
     * @return New LayerConfiguration with updated gain
     */
    LayerConfiguration withGainLevel(float newGain) const;
    
    /**
     * @brief Create a modified copy with new pan position
     * @param newPan New pan position (-1.0 to 1.0)
     * @return New LayerConfiguration with updated pan
     */
    LayerConfiguration withPanPosition(float newPan) const;
    
    /**
     * @brief Create a modified copy with new enabled state
     * @param enabled New enabled state
     * @return New LayerConfiguration with updated state
     */
    LayerConfiguration withEnabledState(bool enabled) const;
    
    /**
     * @brief Create a modified copy with new priority
     * @param priority New priority value
     * @return New LayerConfiguration with updated priority
     */
    LayerConfiguration withPriority(int priority) const;
    
    /**
     * @brief Export layer configuration to JSON
     * @return JSON representation of layer
     */
    nlohmann::json exportToJson() const;
    
    /**
     * @brief Load layer configuration from JSON
     * @param jsonData JSON object containing layer data
     * @param instrumentConfig Instrument configuration to associate
     * @return Loaded LayerConfiguration
     */
    static LayerConfiguration loadFromJson(
        const nlohmann::json& jsonData,
        std::shared_ptr<const InstrumentConfig> instrumentConfig
    );

private:
    LayerId layerId_;
    LayerRole layerRole_;
    std::shared_ptr<const InstrumentConfig> instrumentConfig_;
    float gainLevel_;
    float panPosition_{0.0f};
    bool isEnabled_{true};
    int priority_{0};
    
    /**
     * @brief Validate layer parameters
     * @param gain Gain level to validate
     * @param pan Pan position to validate
     * @return true if parameters are valid
     */
    static bool validateParameters(float gain, float pan) noexcept;
};

/**
 * @brief Composite patch configuration with multiple layers
 * 
 * Manages a complete multi-layer patch configuration with layer
 * management, balance control, and immutable operations.
 */
class CompositePatchConfiguration {
public:
    /**
     * @brief Construct empty composite patch
     * @param patchId Unique identifier for this patch
     * @param patchName Human-readable patch name
     */
    explicit CompositePatchConfiguration(
        ConfigurationId patchId,
        std::string patchName = ""
    );
    
    /**
     * @brief Get patch identifier
     * @return Patch configuration ID
     */
    const ConfigurationId& getPatchId() const noexcept { return patchId_; }
    
    /**
     * @brief Get patch name
     * @return Human-readable patch name
     */
    const std::string& getPatchName() const noexcept { return patchName_; }
    
    /**
     * @brief Get all layers in priority order
     * @return Vector of layer configurations sorted by priority
     */
    std::vector<LayerConfiguration> getLayers() const noexcept;
    
    /**
     * @brief Get layers by role
     * @param role Layer role to filter by
     * @return Vector of layers with matching role
     */
    std::vector<LayerConfiguration> getLayersByRole(LayerRole role) const noexcept;
    
    /**
     * @brief Get layer by ID
     * @param layerId Layer ID to find
     * @return Optional layer configuration if found
     */
    std::optional<LayerConfiguration> getLayerById(const LayerId& layerId) const noexcept;
    
    /**
     * @brief Check if patch has base instrument
     * @return true if base instrument is configured
     */
    bool hasBaseInstrument() const noexcept { return baseInstrument_ != nullptr; }
    
    /**
     * @brief Get base instrument configuration
     * @return Shared pointer to base instrument (may be null)
     */
    std::shared_ptr<const InstrumentConfig> getBaseInstrument() const noexcept { return baseInstrument_; }
    
    /**
     * @brief Get base instrument gain level
     * @return Base instrument gain (0.0 to 1.0)
     */
    float getBaseInstrumentGain() const noexcept { return baseInstrumentGain_; }
    
    /**
     * @brief Get total number of layers
     * @return Layer count
     */
    std::size_t getLayerCount() const noexcept { return layers_.size(); }
    
    /**
     * @brief Get total number of active layers
     * @return Active layer count
     */
    std::size_t getActiveLayerCount() const noexcept;
    
    /**
     * @brief Get master gain for entire patch
     * @return Master gain level (0.0 to 1.0)
     */
    float getMasterGain() const noexcept { return masterGain_; }
    
    /**
     * @brief Create new patch with added layer
     * @param layer Layer configuration to add
     * @return New CompositePatchConfiguration with added layer
     */
    CompositePatchConfiguration withAddedLayer(const LayerConfiguration& layer) const;
    
    /**
     * @brief Create new patch with removed layer
     * @param layerId ID of layer to remove
     * @return New CompositePatchConfiguration without specified layer
     */
    CompositePatchConfiguration withRemovedLayer(const LayerId& layerId) const;
    
    /**
     * @brief Create new patch with updated layer
     * @param layerId ID of layer to update
     * @param updatedLayer New layer configuration
     * @return New CompositePatchConfiguration with updated layer
     */
    CompositePatchConfiguration withUpdatedLayer(
        const LayerId& layerId,
        const LayerConfiguration& updatedLayer
    ) const;
    
    /**
     * @brief Create new patch with base instrument
     * @param baseInstrument Base instrument configuration
     * @param gain Base instrument gain level
     * @return New CompositePatchConfiguration with base instrument
     */
    CompositePatchConfiguration withBaseInstrument(
        std::shared_ptr<const InstrumentConfig> baseInstrument,
        float gain = 0.8f
    ) const;
    
    /**
     * @brief Create new patch with updated master gain
     * @param newMasterGain New master gain level (0.0 to 1.0)
     * @return New CompositePatchConfiguration with updated gain
     */
    CompositePatchConfiguration withMasterGain(float newMasterGain) const;
    
    /**
     * @brief Create new patch with balanced layer gains
     * @param balanceStrategy Strategy for gain balancing
     * @param musicalContext Context for balance decisions
     * @return New CompositePatchConfiguration with balanced gains
     */
    CompositePatchConfiguration withBalancedGains(
        const std::string& balanceStrategy = "automatic",
        const std::map<std::string, std::string>& musicalContext = {}
    ) const;
    
    /**
     * @brief Export complete patch to JSON
     * @param includeMetadata Whether to include metadata in output
     * @return JSON representation of patch
     */
    nlohmann::json exportToJson(bool includeMetadata = true) const;
    
    /**
     * @brief Load patch configuration from JSON
     * @param jsonData JSON object containing patch data
     * @param instrumentConfigs Available instrument configurations
     * @return Loaded CompositePatchConfiguration
     */
    static CompositePatchConfiguration loadFromJson(
        const nlohmann::json& jsonData,
        const std::map<ConfigurationId, std::shared_ptr<const InstrumentConfig>>& instrumentConfigs
    );
    
    /**
     * @brief Validate patch configuration
     * @return true if patch is valid and renderable
     */
    bool validatePatch() const noexcept;

private:
    ConfigurationId patchId_;
    std::string patchName_;
    std::vector<LayerConfiguration> layers_;
    std::shared_ptr<const InstrumentConfig> baseInstrument_;
    float baseInstrumentGain_{0.8f};
    float masterGain_{1.0f};
    
    /**
     * @brief Sort layers by priority
     * @param layers Vector of layers to sort
     * @return Sorted vector of layers
     */
    static std::vector<LayerConfiguration> sortLayersByPriority(
        const std::vector<LayerConfiguration>& layers
    );
    
    /**
     * @brief Validate gain parameters
     * @param gain Gain value to validate
     * @return true if gain is valid
     */
    static bool validateGain(float gain) noexcept;
};

/**
 * @brief Layer assignment strategy for AI-driven layer selection
 */
enum class LayerAssignmentStrategy {
    AUTOMATIC,          ///< AI determines best layer assignment
    ENVELOPE_BASED,     ///< Assign based on envelope characteristics
    TIMBRAL_BASED,      ///< Assign based on timbral characteristics
    MANUAL_PRIORITY,    ///< Use manually specified priorities
    FREQUENCY_BASED     ///< Assign based on frequency content
};

/**
 * @brief Gain balancing strategy for layer mixing
 */
enum class GainBalancingStrategy {
    AUTOMATIC,              ///< AI-driven automatic balancing
    MUSICAL_CONTEXT,        ///< Balance based on musical context
    EQUAL_WEIGHT,           ///< Equal weight for all layers
    ROLE_BASED,             ///< Balance based on layer roles
    DYNAMIC_RANGE_AWARE     ///< Consider dynamic range characteristics
};

/**
 * @brief AI-driven patch generator
 * 
 * Generates composite patches from user intent and available configurations.
 * Implements layering strategies, gain balancing, and context-aware
 * patch construction with extensible AI algorithms.
 */
class AiPatchGenerator {
public:
    /**
     * @brief Patch generation request parameters
     */
    struct GenerationRequest {
        std::vector<std::string> userTags{};                           ///< User-provided semantic tags
        std::map<std::string, std::string> musicalContext{};           ///< Musical context (section, mood, etc.)
        std::size_t maxLayers{6};                                      ///< Maximum number of layers
        float minimumScore{0.2f};                                      ///< Minimum configuration score
        bool includeBaseInstrument{true};                              ///< Whether to include base instrument
        LayerAssignmentStrategy layerStrategy{LayerAssignmentStrategy::AUTOMATIC};  ///< Layer assignment strategy
        GainBalancingStrategy balanceStrategy{GainBalancingStrategy::AUTOMATIC};    ///< Gain balancing strategy
        std::string patchName{};                                       ///< Desired patch name (auto-generated if empty)
    };
    
    /**
     * @brief Patch generation result
     */
    struct GenerationResult {
        CompositePatchConfiguration patch{ConfigurationId(""), ""};    ///< Generated patch
        std::vector<AiConfigurationScorer::ScoringResult> sourceScores{}; ///< Scores for source configurations
        std::string generationReason{};                                ///< Explanation of generation decisions
        std::map<LayerRole, float> layerRoleWeights{};                 ///< Computed weights by role
        bool isHighQuality{false};                                     ///< Whether patch meets quality threshold
    };
    
    /**
     * @brief Construct AI patch generator
     * @param keywordDatabase Reference to semantic keyword database
     * @param suggestionEngine Reference to configuration suggestion engine
     */
    explicit AiPatchGenerator(
        const SemanticKeywordDatabase& keywordDatabase,
        const ConfigurationSuggestionEngine& suggestionEngine
    );
    
    /**
     * @brief Generate composite patch from user request
     * @param request Generation request parameters
     * @param availableConfigurations Available configurations to use
     * @return Generation result with composite patch
     */
    GenerationResult generatePatch(
        const GenerationRequest& request,
        const std::vector<std::reference_wrapper<const InstrumentConfig>>& availableConfigurations
    ) const noexcept;
    
    /**
     * @brief Generate smart patch with minimal user input
     * @param userTags User-provided semantic tags
     * @param availableConfigurations Available configurations
     * @param patchName Desired patch name (optional)
     * @return Generation result with auto-configured patch
     */
    GenerationResult generateSmartPatch(
        const std::vector<std::string>& userTags,
        const std::vector<std::reference_wrapper<const InstrumentConfig>>& availableConfigurations,
        const std::string& patchName = ""
    ) const noexcept;
    
    /**
     * @brief Balance gains for existing patch
     * @param patch Patch to balance
     * @param strategy Balancing strategy
     * @param musicalContext Musical context for balancing
     * @return New patch with balanced gains
     */
    CompositePatchConfiguration balanceLayerGains(
        const CompositePatchConfiguration& patch,
        GainBalancingStrategy strategy,
        const std::map<std::string, std::string>& musicalContext = {}
    ) const noexcept;
    
    /**
     * @brief Assign layer roles to configurations
     * @param configurations Configurations to assign roles
     * @param strategy Assignment strategy
     * @param userTags User tags for context
     * @return Map of configuration ID to assigned layer role
     */
    std::map<ConfigurationId, LayerRole> assignLayerRoles(
        const std::vector<std::reference_wrapper<const InstrumentConfig>>& configurations,
        LayerAssignmentStrategy strategy,
        const std::vector<std::string>& userTags = {}
    ) const noexcept;
    
    /**
     * @brief Generate automatic patch name from content
     * @param userTags User tags used for generation
     * @param patch Generated patch configuration
     * @return Auto-generated patch name
     */
    std::string generatePatchName(
        const std::vector<std::string>& userTags,
        const CompositePatchConfiguration& patch
    ) const noexcept;

private:
    const SemanticKeywordDatabase& keywordDatabase_;
    const ConfigurationSuggestionEngine& suggestionEngine_;
    
    /**
     * @brief Select base instrument from available configurations
     * @param request Generation request
     * @param availableConfigurations Available configurations
     * @return Selected base instrument configuration (may be null)
     */
    std::shared_ptr<const InstrumentConfig> selectBaseInstrument(
        const GenerationRequest& request,
        const std::vector<std::reference_wrapper<const InstrumentConfig>>& availableConfigurations
    ) const noexcept;
    
    /**
     * @brief Select layer configurations from available options
     * @param request Generation request
     * @param availableConfigurations Available configurations
     * @param excludeBaseInstrument Base instrument to exclude from layers
     * @return Vector of selected configurations for layers
     */
    std::vector<std::reference_wrapper<const InstrumentConfig>> selectLayerConfigurations(
        const GenerationRequest& request,
        const std::vector<std::reference_wrapper<const InstrumentConfig>>& availableConfigurations,
        std::shared_ptr<const InstrumentConfig> excludeBaseInstrument = nullptr
    ) const noexcept;
    
    /**
     * @brief Assign layer role based on configuration characteristics
     * @param config Configuration to analyze
     * @param strategy Assignment strategy
     * @param userTags User tags for context
     * @return Assigned layer role
     */
    LayerRole assignSingleLayerRole(
        const InstrumentConfig& config,
        LayerAssignmentStrategy strategy,
        const std::vector<std::string>& userTags
    ) const noexcept;
    
    /**
     * @brief Compute base gain for layer role
     * @param role Layer role
     * @param strategy Balancing strategy
     * @return Base gain value (0.0 to 1.0)
     */
    float computeBaseGainForRole(LayerRole role, GainBalancingStrategy strategy) const noexcept;
    
    /**
     * @brief Apply musical context modifiers to gain
     * @param baseGain Base gain value
     * @param role Layer role
     * @param musicalContext Musical context map
     * @return Modified gain value
     */
    float applyMusicalContextModifiers(
        float baseGain,
        LayerRole role,
        const std::map<std::string, std::string>& musicalContext
    ) const noexcept;
    
    /**
     * @brief Apply cross-layer ducking and interaction
     * @param layers Vector of layer configurations
     * @return Vector of layers with adjusted gains
     */
    std::vector<LayerConfiguration> applyCrossLayerInteraction(
        const std::vector<LayerConfiguration>& layers
    ) const noexcept;
    
    /**
     * @brief Validate generation result quality
     * @param result Generation result to validate
     * @return true if result meets quality standards
     */
    bool validateGenerationQuality(const GenerationResult& result) const noexcept;
    
    /**
     * @brief Generate explanation of generation decisions
     * @param request Original request
     * @param result Generation result
     * @return Human-readable explanation
     */
    std::string generateExplanation(
        const GenerationRequest& request,
        const GenerationResult& result
    ) const noexcept;
};

/**
 * @brief Output configuration manager
 * 
 * Manages the generation of final output configurations that separate
 * reference data (moods.json, Synthesizer.json) from renderable
 * configurations (group.json, guitar.json).
 */
class OutputConfigurationManager {
public:
    /**
     * @brief Output format options
     */
    enum class OutputFormat {
        GROUPED_JSON,       ///< Separate group.json and guitar.json
        UNIFIED_JSON,       ///< Single combined JSON file
        LAYERED_JSON,       ///< Layered patch format
        MODULAR_JSON        ///< Modular format with imports
    };
    
    /**
     * @brief Output configuration parameters
     */
    struct OutputConfiguration {
        OutputFormat format{OutputFormat::GROUPED_JSON};       ///< Output format
        bool includeOnlyValid{true};                           ///< Only include valid configurations
        bool includeMetadata{true};                            ///< Include metadata in output
        bool compressEmptyValues{true};                        ///< Remove empty/null values
        std::string outputDirectory{"."};                      ///< Output directory path
        std::string filenamePrefix{""};                        ///< Optional filename prefix
    };
    
    /**
     * @brief Construct output configuration manager
     */
    explicit OutputConfigurationManager() = default;
    
    /**
     * @brief Export configurations to files
     * @param configurations Map of configurations to export
     * @param patches Vector of composite patches to export
     * @param outputConfig Output configuration parameters
     * @return true if export was successful
     */
    bool exportConfigurations(
        const std::map<ConfigurationId, std::shared_ptr<const InstrumentConfig>>& configurations,
        const std::vector<CompositePatchConfiguration>& patches,
        const OutputConfiguration& outputConfig
    ) const noexcept;
    
    /**
     * @brief Export single patch to JSON
     * @param patch Patch to export
     * @param outputConfig Output configuration parameters
     * @param filename Output filename
     * @return true if export was successful
     */
    bool exportPatch(
        const CompositePatchConfiguration& patch,
        const OutputConfiguration& outputConfig,
        const std::string& filename
    ) const noexcept;
    
    /**
     * @brief Validate configurations before export
     * @param configurations Configurations to validate
     * @param patches Patches to validate
     * @return true if all configurations are valid for export
     */
    bool validateForExport(
        const std::map<ConfigurationId, std::shared_ptr<const InstrumentConfig>>& configurations,
        const std::vector<CompositePatchConfiguration>& patches
    ) const noexcept;

private:
    /**
     * @brief Group configurations by instrument type
     * @param configurations Configurations to group
     * @return Map of instrument type to configurations
     */
    std::map<InstrumentType, std::vector<std::shared_ptr<const InstrumentConfig>>> groupByInstrumentType(
        const std::map<ConfigurationId, std::shared_ptr<const InstrumentConfig>>& configurations
    ) const noexcept;
    
    /**
     * @brief Clean JSON output by removing empty values
     * @param jsonData JSON to clean
     * @return Cleaned JSON object
     */
    nlohmann::json cleanJsonOutput(const nlohmann::json& jsonData) const noexcept;
    
    /**
     * @brief Generate filename for output
     * @param instrumentType Instrument type
     * @param outputConfig Output configuration
     * @return Generated filename
     */
    std::string generateFilename(
        InstrumentType instrumentType,
        const OutputConfiguration& outputConfig
    ) const noexcept;
};

} // namespace AISynthesis