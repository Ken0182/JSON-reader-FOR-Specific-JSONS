#pragma once

#include "common_types.h"
#include "audio_config.h"
#include "ai_scorer.h"
#include "patch_generator.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

/**
 * @file interactive_menu.h
 * @brief Interactive menu system for AI-driven configuration selection
 * @author AI Synthesis Team
 * @version 1.0
 */

namespace AISynthesis {

/**
 * @brief Menu choice with description and validation
 */
struct MenuChoice {
    std::string key{};                  ///< Choice identifier/key
    std::string displayText{};          ///< Text to display to user
    std::string description{};          ///< Detailed description
    bool isValid{true};                 ///< Whether choice is currently valid
    
    /**
     * @brief Construct menu choice
     * @param choiceKey Choice identifier
     * @param display Display text
     * @param desc Description (optional)
     */
    explicit MenuChoice(
        std::string choiceKey,
        std::string display,
        std::string desc = ""
    ) : key(std::move(choiceKey)), displayText(std::move(display)), description(std::move(desc)) {}
};

/**
 * @brief Menu section with grouped choices
 */
struct MenuSection {
    std::string title{};                        ///< Section title
    std::string description{};                  ///< Section description
    std::vector<MenuChoice> choices{};          ///< Available choices
    bool allowMultipleSelection{false};         ///< Whether multiple choices can be selected
    bool isRequired{true};                      ///< Whether selection is required
    
    /**
     * @brief Add choice to section
     * @param choice Menu choice to add
     */
    void addChoice(MenuChoice choice) { choices.push_back(std::move(choice)); }
    
    /**
     * @brief Find choice by key
     * @param key Choice key to find
     * @return Pointer to choice if found, nullptr otherwise
     */
    const MenuChoice* findChoice(const std::string& key) const noexcept;
    
    /**
     * @brief Get all valid choices
     * @return Vector of valid choices
     */
    std::vector<MenuChoice> getValidChoices() const noexcept;
};

/**
 * @brief User selection state during menu interaction
 */
struct UserSelectionState {
    std::string currentSection{};                                   ///< Current menu section
    std::map<std::string, std::vector<std::string>> selections{};   ///< Selections by section
    std::map<std::string, std::string> musicalContext{};           ///< Derived musical context
    std::vector<std::string> allSelectedTags{};                    ///< All selected tags combined
    bool isComplete{false};                                         ///< Whether selection is complete
    
    /**
     * @brief Add selection to current section
     * @param section Section name
     * @param choice Choice key
     */
    void addSelection(const std::string& section, const std::string& choice);
    
    /**
     * @brief Remove selection from section
     * @param section Section name
     * @param choice Choice key to remove
     */
    void removeSelection(const std::string& section, const std::string& choice);
    
    /**
     * @brief Get selections for a section
     * @param section Section name
     * @return Vector of selected choice keys
     */
    std::vector<std::string> getSelectionsForSection(const std::string& section) const noexcept;
    
    /**
     * @brief Clear all selections
     */
    void clear() noexcept;
    
    /**
     * @brief Update all selected tags from current selections
     */
    void updateAllSelectedTags();
};

/**
 * @brief Menu navigation and display interface
 * 
 * Handles the presentation of menu options, user input processing,
 * and navigation between menu sections. Provides extensible interface
 * for different UI backends.
 */
class MenuInterface {
public:
    /**
     * @brief Virtual destructor for proper inheritance
     */
    virtual ~MenuInterface() = default;
    
    /**
     * @brief Display menu section to user
     * @param section Menu section to display
     * @param selectionState Current user selection state
     */
    virtual void displaySection(const MenuSection& section, const UserSelectionState& selectionState) = 0;
    
    /**
     * @brief Get user input for current section
     * @param section Current menu section
     * @param prompt Input prompt to display
     * @return User input string
     */
    virtual std::string getUserInput(const MenuSection& section, const std::string& prompt) = 0;
    
    /**
     * @brief Display suggestions to user
     * @param suggestions Vector of suggestions with scores
     * @param context Context for suggestions
     */
    virtual void displaySuggestions(
        const std::vector<AiConfigurationScorer::ScoringResult>& suggestions,
        const std::string& context
    ) = 0;
    
    /**
     * @brief Display generation result to user
     * @param result Patch generation result
     */
    virtual void displayGenerationResult(const AiPatchGenerator::GenerationResult& result) = 0;
    
    /**
     * @brief Display error message to user
     * @param message Error message
     * @param context Error context
     */
    virtual void displayError(const std::string& message, const std::string& context = "") = 0;
    
    /**
     * @brief Display informational message to user
     * @param message Information message
     */
    virtual void displayInfo(const std::string& message) = 0;
    
    /**
     * @brief Ask user for confirmation
     * @param question Question to ask
     * @param defaultAnswer Default answer if user presses enter
     * @return true if user confirms, false otherwise
     */
    virtual bool askConfirmation(const std::string& question, bool defaultAnswer = false) = 0;
    
    /**
     * @brief Display progress indicator
     * @param message Progress message
     * @param percentage Progress percentage (0-100)
     */
    virtual void displayProgress(const std::string& message, int percentage) = 0;
    
    /**
     * @brief Clear the display
     */
    virtual void clearDisplay() = 0;
    
    /**
     * @brief Wait for user to press enter
     * @param message Message to display
     */
    virtual void waitForUser(const std::string& message = "Press Enter to continue...") = 0;
};

/**
 * @brief Console-based menu interface implementation
 * 
 * Implements MenuInterface for console/terminal-based interaction
 * with colored output, formatted tables, and keyboard input.
 */
class ConsoleMenuInterface : public MenuInterface {
public:
    /**
     * @brief Construct console menu interface
     * @param useColors Whether to use colored output
     */
    explicit ConsoleMenuInterface(bool useColors = true);
    
    // MenuInterface implementation
    void displaySection(const MenuSection& section, const UserSelectionState& selectionState) override;
    std::string getUserInput(const MenuSection& section, const std::string& prompt) override;
    void displaySuggestions(
        const std::vector<AiConfigurationScorer::ScoringResult>& suggestions,
        const std::string& context
    ) override;
    void displayGenerationResult(const AiPatchGenerator::GenerationResult& result) override;
    void displayError(const std::string& message, const std::string& context = "") override;
    void displayInfo(const std::string& message) override;
    bool askConfirmation(const std::string& question, bool defaultAnswer = false) override;
    void displayProgress(const std::string& message, int percentage) override;
    void clearDisplay() override;
    void waitForUser(const std::string& message = "Press Enter to continue...") override;
    
    /**
     * @brief Set whether to use colored output
     * @param useColors true to enable colors
     */
    void setUseColors(bool useColors) noexcept { useColors_ = useColors; }
    
    /**
     * @brief Set page size for pagination
     * @param pageSize Number of items per page
     */
    void setPageSize(std::size_t pageSize) noexcept { pageSize_ = pageSize; }

private:
    bool useColors_;
    std::size_t pageSize_{10};
    
    /**
     * @brief Format text with color codes
     * @param text Text to format
     * @param colorCode ANSI color code
     * @return Formatted text
     */
    std::string formatWithColor(const std::string& text, const std::string& colorCode) const noexcept;
    
    /**
     * @brief Display paginated choices
     * @param choices Vector of choices to display
     * @param selectedChoices Currently selected choices
     */
    void displayPaginatedChoices(
        const std::vector<MenuChoice>& choices,
        const std::vector<std::string>& selectedChoices
    ) const;
    
    /**
     * @brief Format suggestion score as visual bar
     * @param score Score value (0.0 to 1.0)
     * @param barLength Length of bar in characters
     * @return Visual score bar
     */
    std::string formatScoreBar(float score, int barLength = 20) const noexcept;
};

/**
 * @brief Menu flow controller
 * 
 * Manages the overall menu flow, section navigation, and user experience.
 * Coordinates between menu interface, AI systems, and configuration management.
 */
class MenuFlowController {
public:
    /**
     * @brief Menu completion callback function type
     */
    using CompletionCallback = std::function<void(const UserSelectionState&, const AiPatchGenerator::GenerationResult&)>;
    
    /**
     * @brief Construct menu flow controller
     * @param menuInterface Reference to menu interface
     * @param keywordDatabase Reference to semantic keyword database
     * @param suggestionEngine Reference to configuration suggestion engine
     * @param patchGenerator Reference to AI patch generator
     */
    explicit MenuFlowController(
        std::unique_ptr<MenuInterface> menuInterface,
        const SemanticKeywordDatabase& keywordDatabase,
        const ConfigurationSuggestionEngine& suggestionEngine,
        const AiPatchGenerator& patchGenerator
    );
    
    /**
     * @brief Start interactive menu session
     * @param availableConfigurations Available configurations for selection
     * @param completionCallback Optional callback for when menu completes
     */
    void startMenuSession(
        const std::vector<std::reference_wrapper<const InstrumentConfig>>& availableConfigurations,
        CompletionCallback completionCallback = nullptr
    );
    
    /**
     * @brief Add menu section
     * @param sectionId Section identifier
     * @param section Menu section to add
     */
    void addMenuSection(const std::string& sectionId, MenuSection section);
    
    /**
     * @brief Set section order for navigation
     * @param sectionOrder Vector of section IDs in navigation order
     */
    void setSectionOrder(const std::vector<std::string>& sectionOrder);
    
    /**
     * @brief Enable/disable AI suggestions during selection
     * @param enabled true to enable AI suggestions
     */
    void setAiSuggestionsEnabled(bool enabled) noexcept { aiSuggestionsEnabled_ = enabled; }
    
    /**
     * @brief Set minimum score threshold for suggestions
     * @param threshold Minimum score (0.0 to 1.0)
     */
    void setSuggestionThreshold(float threshold) noexcept { suggestionThreshold_ = threshold; }
    
    /**
     * @brief Set maximum number of suggestions to show
     * @param maxSuggestions Maximum number of suggestions
     */
    void setMaxSuggestions(std::size_t maxSuggestions) noexcept { maxSuggestions_ = maxSuggestions; }

private:
    std::unique_ptr<MenuInterface> menuInterface_;
    const SemanticKeywordDatabase& keywordDatabase_;
    const ConfigurationSuggestionEngine& suggestionEngine_;
    const AiPatchGenerator& patchGenerator_;
    
    std::map<std::string, MenuSection> menuSections_;
    std::vector<std::string> sectionOrder_;
    UserSelectionState selectionState_;
    std::vector<std::reference_wrapper<const InstrumentConfig>> availableConfigurations_;
    
    bool aiSuggestionsEnabled_{true};
    float suggestionThreshold_{0.2f};
    std::size_t maxSuggestions_{5};
    
    /**
     * @brief Process user input for current section
     * @param input User input string
     * @param currentSection Current menu section
     * @return true if input was valid and processed
     */
    bool processUserInput(const std::string& input, const MenuSection& currentSection);
    
    /**
     * @brief Navigate to next section
     * @return true if navigation was successful
     */
    bool navigateToNextSection();
    
    /**
     * @brief Navigate to previous section
     * @return true if navigation was successful
     */
    bool navigateToPreviousSection();
    
    /**
     * @brief Update AI suggestions based on current selections
     */
    void updateAiSuggestions();
    
    /**
     * @brief Finalize selection and generate patch
     * @param completionCallback Callback to call with results
     */
    void finalizeSelection(CompletionCallback completionCallback);
    
    /**
     * @brief Validate current selection state
     * @return true if selections are valid and complete
     */
    bool validateSelectionState() const noexcept;
    
    /**
     * @brief Get current section index
     * @return Index of current section in section order
     */
    std::size_t getCurrentSectionIndex() const noexcept;
    
    /**
     * @brief Handle special commands (back, help, quit, etc.)
     * @param input User input to check for commands
     * @return true if input was a special command
     */
    bool handleSpecialCommands(const std::string& input);
    
    /**
     * @brief Display help information
     */
    void displayHelp() const;
    
    /**
     * @brief Display current selection summary
     */
    void displaySelectionSummary() const;
};

/**
 * @brief Interactive menu system for configuration selection
 * 
 * High-level interface that coordinates all menu components and provides
 * a complete interactive experience for users to select and generate
 * AI-driven instrument configurations.
 */
class InteractiveMenuSystem {
public:
    /**
     * @brief Menu configuration options
     */
    struct MenuConfiguration {
        bool enableColors{true};                    ///< Enable colored console output
        bool enableAiSuggestions{true};             ///< Enable AI-driven suggestions
        float suggestionThreshold{0.2f};            ///< Minimum score for suggestions
        std::size_t maxSuggestions{5};              ///< Maximum suggestions to display
        std::size_t pageSize{10};                   ///< Items per page for pagination
        bool autoAdvanceOnSingleChoice{true};      ///< Auto-advance if only one valid choice
        bool showProgressIndicators{true};         ///< Show progress during AI processing
        std::string outputDirectory{"."};          ///< Directory for output files
    };
    
    /**
     * @brief Construct interactive menu system
     * @param keywordDatabase Reference to semantic keyword database
     * @param suggestionEngine Reference to configuration suggestion engine
     * @param patchGenerator Reference to AI patch generator
     * @param config Menu configuration options
     */
    explicit InteractiveMenuSystem(
        const SemanticKeywordDatabase& keywordDatabase,
        const ConfigurationSuggestionEngine& suggestionEngine,
        const AiPatchGenerator& patchGenerator,
        MenuConfiguration config = {}
    );
    
    /**
     * @brief Run complete interactive menu session
     * @param availableConfigurations Available configurations for selection
     * @return true if session completed successfully
     */
    bool runMenuSession(
        const std::vector<std::reference_wrapper<const InstrumentConfig>>& availableConfigurations
    );
    
    /**
     * @brief Initialize default menu structure
     * 
     * Sets up the standard menu flow with sections for:
     * - Musical section selection (intro, verse, chorus, etc.)
     * - Mood selection (calm, energetic, nostalgic, etc.)
     * - Timbral characteristics (warm, bright, dark, etc.)
     * - Instrument preferences (guitar, synth, bass, etc.)
     * - Effect preferences (reverb, delay, distortion, etc.)
     */
    void initializeDefaultMenuStructure();
    
    /**
     * @brief Add custom menu section
     * @param sectionId Section identifier
     * @param title Section title
     * @param description Section description
     * @param choices Available choices for this section
     * @param allowMultiple Whether multiple selections are allowed
     * @param isRequired Whether selection is required
     */
    void addCustomSection(
        const std::string& sectionId,
        const std::string& title,
        const std::string& description,
        const std::vector<MenuChoice>& choices,
        bool allowMultiple = false,
        bool isRequired = true
    );
    
    /**
     * @brief Set menu configuration
     * @param config New menu configuration
     */
    void setMenuConfiguration(const MenuConfiguration& config);
    
    /**
     * @brief Get current menu configuration
     * @return Current menu configuration
     */
    const MenuConfiguration& getMenuConfiguration() const noexcept { return config_; }
    
    /**
     * @brief Export generated patch to file
     * @param result Patch generation result to export
     * @param filename Output filename (optional, auto-generated if empty)
     * @return true if export was successful
     */
    bool exportPatchToFile(
        const AiPatchGenerator::GenerationResult& result,
        const std::string& filename = ""
    ) const noexcept;

private:
    const SemanticKeywordDatabase& keywordDatabase_;
    const ConfigurationSuggestionEngine& suggestionEngine_;
    const AiPatchGenerator& patchGenerator_;
    MenuConfiguration config_;
    
    std::unique_ptr<MenuFlowController> flowController_;
    std::unique_ptr<OutputConfigurationManager> outputManager_;
    
    /**
     * @brief Create default musical section choices
     * @return Vector of musical section menu choices
     */
    std::vector<MenuChoice> createMusicalSectionChoices() const;
    
    /**
     * @brief Create default mood choices
     * @return Vector of mood menu choices
     */
    std::vector<MenuChoice> createMoodChoices() const;
    
    /**
     * @brief Create default timbral choices
     * @return Vector of timbral characteristic choices
     */
    std::vector<MenuChoice> createTimbralChoices() const;
    
    /**
     * @brief Create default instrument choices
     * @return Vector of instrument type choices
     */
    std::vector<MenuChoice> createInstrumentChoices() const;
    
    /**
     * @brief Create default effect choices
     * @return Vector of effect type choices
     */
    std::vector<MenuChoice> createEffectChoices() const;
    
    /**
     * @brief Handle completion callback from menu flow
     * @param selectionState Final user selection state
     * @param generationResult Patch generation result
     */
    void handleMenuCompletion(
        const UserSelectionState& selectionState,
        const AiPatchGenerator::GenerationResult& generationResult
    );
    
    /**
     * @brief Generate automatic filename for patch export
     * @param selectionState User selection state
     * @param result Generation result
     * @return Generated filename
     */
    std::string generateAutomaticFilename(
        const UserSelectionState& selectionState,
        const AiPatchGenerator::GenerationResult& result
    ) const noexcept;
};

} // namespace AISynthesis