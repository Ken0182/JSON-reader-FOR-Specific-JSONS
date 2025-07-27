#include "common_types.h"
#include "audio_config.h"
#include "json_parser.h"
#include "ai_scorer.h"
#include "patch_generator.h"
#include "interactive_menu.h"

#include <iostream>
#include <memory>
#include <vector>
#include <map>
#include <cstdlib>
#include <ctime>

/**
 * @file main.cpp
 * @brief Main entry point for AI-driven instrument synthesis system
 * @author AI Synthesis Team
 * @version 1.0
 */

using namespace AISynthesis;

/**
 * @brief Application configuration loaded from command line or config file
 */
struct ApplicationConfiguration {
    std::string configDirectory{"."};           ///< Directory containing JSON config files
    std::string outputDirectory{"."};           ///< Directory for output files
    std::string keywordDatabaseFile{"skd.json"}; ///< Semantic keyword database file
    bool enableInteractiveMode{true};           ///< Whether to run interactive menu
    bool enableColorsInConsole{true};           ///< Whether to use colored console output
    bool validateConfigurationsOnLoad{true};    ///< Whether to validate configurations during loading
    float minimumSuggestionScore{0.2f};         ///< Minimum score threshold for AI suggestions
    std::size_t maxSuggestions{5};              ///< Maximum number of suggestions to display
    bool verbose{false};                        ///< Enable verbose output
};

/**
 * @brief Parse command line arguments
 * @param argc Argument count
 * @param argv Argument values
 * @return Parsed application configuration
 */
ApplicationConfiguration parseCommandLineArguments(int argc, char* argv[]) {
    ApplicationConfiguration config;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--config-dir" && i + 1 < argc) {
            config.configDirectory = argv[++i];
        } else if (arg == "--output-dir" && i + 1 < argc) {
            config.outputDirectory = argv[++i];
        } else if (arg == "--skd-file" && i + 1 < argc) {
            config.keywordDatabaseFile = argv[++i];
        } else if (arg == "--no-interactive") {
            config.enableInteractiveMode = false;
        } else if (arg == "--no-colors") {
            config.enableColorsInConsole = false;
        } else if (arg == "--no-validation") {
            config.validateConfigurationsOnLoad = false;
        } else if (arg == "--min-score" && i + 1 < argc) {
            config.minimumSuggestionScore = std::stof(argv[++i]);
        } else if (arg == "--max-suggestions" && i + 1 < argc) {
            config.maxSuggestions = std::stoul(argv[++i]);
        } else if (arg == "--verbose" || arg == "-v") {
            config.verbose = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "AI Instrument Synthesis System\n\n"
                      << "Usage: " << argv[0] << " [options]\n\n"
                      << "Options:\n"
                      << "  --config-dir <dir>      Directory containing JSON config files (default: .)\n"
                      << "  --output-dir <dir>      Directory for output files (default: .)\n"
                      << "  --skd-file <file>       Semantic keyword database file (default: skd.json)\n"
                      << "  --no-interactive        Disable interactive menu mode\n"
                      << "  --no-colors             Disable colored console output\n"
                      << "  --no-validation         Skip configuration validation on load\n"
                      << "  --min-score <score>     Minimum suggestion score (0.0-1.0, default: 0.2)\n"
                      << "  --max-suggestions <n>   Maximum suggestions to show (default: 5)\n"
                      << "  --verbose, -v           Enable verbose output\n"
                      << "  --help, -h              Show this help message\n";
            std::exit(0);
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            std::cerr << "Use --help for usage information.\n";
            std::exit(1);
        }
    }
    
    return config;
}

/**
 * @brief Load all configuration files and return loaded configurations
 * @param appConfig Application configuration
 * @return Map of configuration ID to loaded instrument configurations
 */
std::map<ConfigurationId, std::shared_ptr<const InstrumentConfig>> loadAllConfigurations(
    const ApplicationConfiguration& appConfig
) {
    std::map<ConfigurationId, std::shared_ptr<const InstrumentConfig>> allConfigurations;
    ConfigurationFileLoader loader;
    
    // List of configuration files to load
    std::vector<std::pair<std::string, std::string>> configFiles = {
        {"guitar.json", "guitar"},
        {"group.json", "group"},
        {"moods.json", "reference"},
        {"Synthesizer.json", "reference"}
    };
    
    for (const auto& [filename, fileType] : configFiles) {
        std::string fullPath = appConfig.configDirectory + "/" + filename;
        
        if (appConfig.verbose) {
            std::cout << "Loading configuration file: " << fullPath << "\n";
        }
        
        ConfigurationFileLoader::LoadingResult result;
        
        if (fileType == "guitar") {
            result = loader.loadGuitarConfigurations(fullPath);
        } else if (fileType == "group") {
            result = loader.loadGroupConfigurations(fullPath);
        } else if (fileType == "reference") {
            result = loader.loadReferenceConfigurations(fullPath);
        } else {
            result = loader.loadFromFile(fullPath, appConfig.validateConfigurationsOnLoad);
        }
        
        if (result.hasErrors) {
            std::cerr << "Errors loading " << filename << ":\n";
            for (const auto& error : result.errors) {
                std::cerr << "  " << error.getFormattedMessage() << "\n";
            }
        }
        
        if (!result.warnings.empty() && appConfig.verbose) {
            std::cout << "Warnings loading " << filename << ":\n";
            for (const auto& warning : result.warnings) {
                std::cout << "  " << warning.getFormattedMessage() << "\n";
            }
        }
        
        // Add successfully loaded configurations
        for (auto& config : result.configurations) {
            ConfigurationId configId = config->getConfigurationId();
            allConfigurations[configId] = std::shared_ptr<const InstrumentConfig>(config.release());
            
            if (appConfig.verbose) {
                std::cout << "  Loaded configuration: " << configId.getValue() << "\n";
            }
        }
    }
    
    std::cout << "Successfully loaded " << allConfigurations.size() << " configurations.\n";
    return allConfigurations;
}

/**
 * @brief Initialize semantic keyword database
 * @param appConfig Application configuration
 * @return Initialized semantic keyword database
 */
std::unique_ptr<SemanticKeywordDatabase> initializeKeywordDatabase(
    const ApplicationConfiguration& appConfig
) {
    auto keywordDatabase = std::make_unique<SemanticKeywordDatabase>();
    
    std::string skdPath = appConfig.configDirectory + "/" + appConfig.keywordDatabaseFile;
    
    if (appConfig.verbose) {
        std::cout << "Loading semantic keyword database from: " << skdPath << "\n";
    }
    
    if (!keywordDatabase->loadFromFile(skdPath)) {
        std::cout << "Could not load semantic keyword database from file, using default database.\n";
        // Database will initialize with default keywords
    }
    
    if (!keywordDatabase->validateDatabase()) {
        std::cerr << "Warning: Semantic keyword database validation failed. Some features may not work correctly.\n";
    }
    
    if (appConfig.verbose) {
        auto categories = keywordDatabase->getAllCategories();
        auto keywords = keywordDatabase->getAllKeywords();
        std::cout << "Keyword database loaded with " << keywords.size() 
                  << " keywords in " << categories.size() << " categories.\n";
    }
    
    return keywordDatabase;
}

/**
 * @brief Initialize AI systems (scorer, suggestion engine, patch generator)
 * @param keywordDatabase Reference to keyword database
 * @return Tuple of (scorer, suggestion engine, patch generator)
 */
std::tuple<
    std::unique_ptr<AiConfigurationScorer>,
    std::unique_ptr<ConfigurationSuggestionEngine>,
    std::unique_ptr<AiPatchGenerator>
> initializeAiSystems(const SemanticKeywordDatabase& keywordDatabase) {
    
    auto scorer = std::make_unique<AiConfigurationScorer>(
        keywordDatabase,
        AiConfigurationScorer::ScoringStrategy::WEIGHTED_HYBRID
    );
    
    auto suggestionEngine = std::make_unique<ConfigurationSuggestionEngine>(keywordDatabase);
    
    auto patchGenerator = std::make_unique<AiPatchGenerator>(
        keywordDatabase,
        *suggestionEngine
    );
    
    return std::make_tuple(
        std::move(scorer),
        std::move(suggestionEngine),
        std::move(patchGenerator)
    );
}

/**
 * @brief Run interactive menu session
 * @param appConfig Application configuration
 * @param configurations Available configurations
 * @param keywordDatabase Keyword database
 * @param suggestionEngine Suggestion engine
 * @param patchGenerator Patch generator
 * @return true if session completed successfully
 */
bool runInteractiveSession(
    const ApplicationConfiguration& appConfig,
    const std::map<ConfigurationId, std::shared_ptr<const InstrumentConfig>>& configurations,
    const SemanticKeywordDatabase& keywordDatabase,
    const ConfigurationSuggestionEngine& suggestionEngine,
    const AiPatchGenerator& patchGenerator
) {
    // Create menu configuration
    InteractiveMenuSystem::MenuConfiguration menuConfig;
    menuConfig.enableColors = appConfig.enableColorsInConsole;
    menuConfig.enableAiSuggestions = true;
    menuConfig.suggestionThreshold = appConfig.minimumSuggestionScore;
    menuConfig.maxSuggestions = appConfig.maxSuggestions;
    menuConfig.outputDirectory = appConfig.outputDirectory;
    menuConfig.showProgressIndicators = true;
    menuConfig.pageSize = 10;
    
    // Create interactive menu system
    InteractiveMenuSystem menuSystem(
        keywordDatabase,
        suggestionEngine,
        patchGenerator,
        menuConfig
    );
    
    // Initialize default menu structure
    menuSystem.initializeDefaultMenuStructure();
    
    // Convert configurations to reference wrappers for menu system
    std::vector<std::reference_wrapper<const InstrumentConfig>> configRefs;
    for (const auto& [id, config] : configurations) {
        // Only include valid, renderable configurations (not reference-only)
        if (config->getQuality() != ConfigurationQuality::REFERENCE_ONLY) {
            configRefs.emplace_back(*config);
        }
    }
    
    std::cout << "\nStarting interactive configuration selection...\n";
    std::cout << "Available configurations: " << configRefs.size() << "\n\n";
    
    // Run the menu session
    return menuSystem.runMenuSession(configRefs);
}

/**
 * @brief Run non-interactive mode (batch processing or example generation)
 * @param appConfig Application configuration
 * @param configurations Available configurations
 * @param patchGenerator Patch generator
 * @return true if processing completed successfully
 */
bool runNonInteractiveMode(
    const ApplicationConfiguration& appConfig,
    const std::map<ConfigurationId, std::shared_ptr<const InstrumentConfig>>& configurations,
    const AiPatchGenerator& patchGenerator
) {
    std::cout << "Running in non-interactive mode...\n";
    
    // Example: Generate a patch with predefined tags
    std::vector<std::string> exampleTags = {"warm", "calm", "guitar", "reverb", "intro"};
    
    // Convert configurations to reference wrappers
    std::vector<std::reference_wrapper<const InstrumentConfig>> configRefs;
    for (const auto& [id, config] : configurations) {
        if (config->getQuality() != ConfigurationQuality::REFERENCE_ONLY) {
            configRefs.emplace_back(*config);
        }
    }
    
    std::cout << "Generating example patch with tags: ";
    for (const auto& tag : exampleTags) {
        std::cout << tag << " ";
    }
    std::cout << "\n";
    
    // Generate patch
    auto result = patchGenerator.generateSmartPatch(
        exampleTags,
        configRefs,
        "Example_Warm_Calm_Intro"
    );
    
    if (result.isHighQuality) {
        std::cout << "Generated high-quality patch: " << result.patch.getPatchName() << "\n";
        std::cout << "Number of layers: " << result.patch.getLayerCount() << "\n";
        std::cout << "Generation reason: " << result.generationReason << "\n";
        
        // Export to file
        OutputConfigurationManager outputManager;
        OutputConfigurationManager::OutputConfiguration outputConfig;
        outputConfig.outputDirectory = appConfig.outputDirectory;
        outputConfig.format = OutputConfigurationManager::OutputFormat::LAYERED_JSON;
        
        std::string filename = appConfig.outputDirectory + "/example_patch.json";
        bool exportSuccess = outputManager.exportPatch(result.patch, outputConfig, filename);
        
        if (exportSuccess) {
            std::cout << "Patch exported to: " << filename << "\n";
        } else {
            std::cerr << "Failed to export patch to file.\n";
            return false;
        }
    } else {
        std::cout << "Generated patch did not meet quality threshold.\n";
        std::cout << "Generation reason: " << result.generationReason << "\n";
        return false;
    }
    
    return true;
}

/**
 * @brief Main application entry point
 * @param argc Argument count
 * @param argv Argument values
 * @return Exit code (0 for success, non-zero for error)
 */
int main(int argc, char* argv[]) {
    try {
        // Initialize random seed for AI algorithms
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        
        std::cout << "AI Instrument Synthesis System v1.0\n";
        std::cout << "===================================\n\n";
        
        // Parse command line arguments
        ApplicationConfiguration appConfig = parseCommandLineArguments(argc, argv);
        
        if (appConfig.verbose) {
            std::cout << "Configuration:\n";
            std::cout << "  Config directory: " << appConfig.configDirectory << "\n";
            std::cout << "  Output directory: " << appConfig.outputDirectory << "\n";
            std::cout << "  Interactive mode: " << (appConfig.enableInteractiveMode ? "enabled" : "disabled") << "\n";
            std::cout << "  Validation: " << (appConfig.validateConfigurationsOnLoad ? "enabled" : "disabled") << "\n";
            std::cout << "\n";
        }
        
        // Load all configurations
        std::cout << "Loading instrument configurations...\n";
        auto configurations = loadAllConfigurations(appConfig);
        
        if (configurations.empty()) {
            std::cerr << "Error: No valid configurations loaded. Check your configuration files.\n";
            return 1;
        }
        
        // Initialize semantic keyword database
        auto keywordDatabase = initializeKeywordDatabase(appConfig);
        
        // Initialize AI systems
        auto [scorer, suggestionEngine, patchGenerator] = initializeAiSystems(*keywordDatabase);
        
        // Run appropriate mode
        bool success = false;
        
        if (appConfig.enableInteractiveMode) {
            success = runInteractiveSession(
                appConfig,
                configurations,
                *keywordDatabase,
                *suggestionEngine,
                *patchGenerator
            );
        } else {
            success = runNonInteractiveMode(
                appConfig,
                configurations,
                *patchGenerator
            );
        }
        
        if (success) {
            std::cout << "\nApplication completed successfully.\n";
            return 0;
        } else {
            std::cout << "\nApplication completed with errors.\n";
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Fatal error: Unknown exception occurred.\n";
        return 1;
    }
}