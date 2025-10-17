/**
 * @file audio_config_cli.cpp
 * @brief AudioConfigSystem CLI Implementation
 * @author AI Assistant
 * @version 1.0
 */

#include "audio_config_system.hpp"
#include "json.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <map>
#include <ctime>

using json = nlohmann::json;

namespace audio_config {

// ============================================================================
// AudioConfigSystem Implementation
// ============================================================================

AudioConfigSystem::AudioConfigSystem(const std::string& weightsConfigPath)
    : embeddingEngine_(std::make_shared<EmbeddingEngine>())
    , weights_(ScoringWeights::loadFromConfig(weightsConfigPath)) {
    
    pointer_ = std::make_unique<MultiDimensionalPointer>(weights_, embeddingEngine_);
}

bool AudioConfigSystem::initialize(const std::string& configDatabasePath, 
                                    const std::string& skdIndexPath) {
    try {
        // v1.3: Load external SKD embedding index if provided
        if (!skdIndexPath.empty()) {
            std::cout << "Loading SKD embedding index..." << std::endl;
            if (embeddingEngine_->loadEmbeddingIndex(skdIndexPath)) {
                std::cout << "SKD embeddings loaded - using semantically meaningful vectors" << std::endl;
            } else {
                std::cout << "Using built-in vocabulary (fallback)" << std::endl;
            }
        }
        
        // Load configuration database
        loadConfigurationDatabase(configDatabasePath);
        
        // v1.2/v1.3: Calculate IDF statistics for all tags
        std::vector<std::vector<std::string>> allTags;
        for (const auto& [id, config] : configurations_) {
            allTags.push_back(config->getSemanticTags());
        }
        embeddingEngine_->updateTagStatistics(allTags);
        
        std::cout << "Loaded " << configurations_.size() << " configurations with multi-dimensional metadata." << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Initialization failed: " << e.what() << std::endl;
        return false;
    }
}

void AudioConfigSystem::loadConfigurationDatabase(const std::string& configPath) {
    std::ifstream file(configPath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open configuration database: " + configPath);
    }
    
    json configDatabase;
    file >> configDatabase;
    
    for (const auto& [configId, configData] : configDatabase.items()) {
        auto configDataPtr = std::make_shared<json>(configData);
        auto audioConfig = std::make_shared<AudioConfig>(configId, configId, configDataPtr);
        
        // Extract and set semantic metadata
        std::vector<std::string> semanticTags;
        if (configData.contains("soundCharacteristics")) {
            const auto& chars = configData["soundCharacteristics"];
            if (chars.contains("timbral") && chars["timbral"].is_string()) {
                semanticTags.push_back(chars["timbral"].get<std::string>());
            }
            if (chars.contains("dynamic") && chars["dynamic"].is_string()) {
                semanticTags.push_back(chars["dynamic"].get<std::string>());
            }
            if (chars.contains("material") && chars["material"].is_string()) {
                semanticTags.push_back(chars["material"].get<std::string>());
            }
            if (chars.contains("emotional") && chars["emotional"].is_array()) {
                for (const auto& emotion : chars["emotional"]) {
                    if (emotion.is_object() && emotion.contains("tag")) {
                        semanticTags.push_back(emotion["tag"].get<std::string>());
                    }
                }
            }
        }
        audioConfig->setSemanticTags(std::move(semanticTags));
        
        // v1.4: Generate embedding from normalized tokens (same as search pipeline)
        std::string embeddingText = TextUtils::normalizeForEmbedding(configId);
        for (const auto& tag : audioConfig->getSemanticTags()) {
            embeddingText += " " + TextUtils::normalizeForEmbedding(tag);
        }
        audioConfig->setEmbedding(embeddingEngine_->getEmbedding(embeddingText));
        
        // Set technical specifications
        TechnicalSpecs techSpecs;
        if (configData.contains("adsr") && configData["adsr"].contains("type")) {
            techSpecs.envelopeType = configData["adsr"]["type"].get<std::string>();
        }
        audioConfig->setTechnicalSpecs(std::move(techSpecs));
        
        // Determine musical role
        MusicalRoleInfo roleInfo;
        std::string nameLower = configId;
        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
        
        if (nameLower.find("lead") != std::string::npos) {
            roleInfo.primaryRole = MusicalRole::Lead;
            roleInfo.prominence = 0.9f;
        } else if (nameLower.find("bass") != std::string::npos) {
            roleInfo.primaryRole = MusicalRole::Bass;
            roleInfo.prominence = 0.7f;
        } else if (nameLower.find("pad") != std::string::npos) {
            roleInfo.primaryRole = MusicalRole::Pad;
            roleInfo.prominence = 0.3f;
        } else if (nameLower.find("arp") != std::string::npos) {
            roleInfo.primaryRole = MusicalRole::Arp;
            roleInfo.prominence = 0.6f;
        } else if (nameLower.find("chord") != std::string::npos) {
            roleInfo.primaryRole = MusicalRole::Chord;
            roleInfo.prominence = 0.5f;
        } else if (configData.contains("guitarParams")) {
            roleInfo.primaryRole = MusicalRole::Lead;
            roleInfo.prominence = 0.8f;
        } else {
            roleInfo.primaryRole = MusicalRole::Pad;
            roleInfo.prominence = 0.4f;
        }
        
        // Set tonal character from sound characteristics
        if (configData.contains("soundCharacteristics") && 
            configData["soundCharacteristics"].contains("timbral")) {
            std::string timbral = configData["soundCharacteristics"]["timbral"].get<std::string>();
            if (timbral == "bright" || timbral == "sharp") {
                roleInfo.tonalCharacter = "bright";
            } else if (timbral == "warm" || timbral == "soft") {
                roleInfo.tonalCharacter = "warm";
            } else if (timbral == "dark" || timbral == "deep") {
                roleInfo.tonalCharacter = "dark";
            }
        }
        
        audioConfig->setMusicalRole(std::move(roleInfo));
        
        // Set layering information
        LayeringInfo layeringInfo;
        if (roleInfo.prominence >= 0.7f) {
            layeringInfo.preferredLayer = ArrangementLayer::Foreground;
        } else if (roleInfo.prominence >= 0.4f) {
            layeringInfo.preferredLayer = ArrangementLayer::Midground;
        } else {
            layeringInfo.preferredLayer = ArrangementLayer::Background;
        }
        
        // Set frequency range based on role
        if (roleInfo.primaryRole == MusicalRole::Bass) {
            layeringInfo.frequencyRange = "low";
        } else if (roleInfo.primaryRole == MusicalRole::Lead) {
            layeringInfo.frequencyRange = "high-mid";
        } else if (roleInfo.primaryRole == MusicalRole::Pad) {
            layeringInfo.frequencyRange = "mid";
        } else {
            layeringInfo.frequencyRange = "full";
        }
        
        layeringInfo.mixPriority = roleInfo.prominence;
        audioConfig->setLayeringInfo(std::move(layeringInfo));
        
        configurations_[configId] = audioConfig;
    }
}

std::vector<std::pair<std::shared_ptr<AudioConfig>, CompatibilityScore>> 
AudioConfigSystem::searchConfigurations(const std::string& query, int maxResults) const {
    std::vector<std::pair<std::shared_ptr<AudioConfig>, CompatibilityScore>> results;
    
    // v1.4: Tokenize query using unified normalization pipeline
    auto queryTokens = TextUtils::tokenize(query);
    
    // v1.5: Record query tokens for interest tracking
    // Note: Using const_cast to allow recording in const method (tracker is mutable state)
    const_cast<UserContext&>(userContext_).getSearchTracker().recordQuery(queryTokens);
    
    // v1.4: Generate query embedding from normalized tokens (aligned with config embeddings)
    std::string normalizedQuery = TextUtils::normalizeForEmbedding(query);
    EmbeddingVector queryEmbedding = embeddingEngine_->getEmbedding(normalizedQuery);
    
    for (const auto& [configId, config] : configurations_) {
        // Skip excluded configurations
        if (userContext_.isExcluded(configId)) continue;
        
        // 1. Calculate semantic similarity (SKD-based cosine)
        float semanticScore = EmbeddingEngine::calculateSimilarity(queryEmbedding, config->getEmbedding());
        
        // 2. v1.4: Per-token matching across IDs and tags
        auto configTokens = config->getAllTokens();
        float tokenOverlap = TextUtils::calculateTokenOverlap(queryTokens, configTokens);
        
        // 3. Boost for exact multi-token matches (e.g., "funky retro" → both tokens present)
        float multiTokenBoost = 0.0f;
        if (queryTokens.size() > 1 && tokenOverlap >= 0.5f) {
            // All query tokens found in config → strong match
            int matchedTokens = 0;
            std::unordered_set<std::string> configTokenSet(configTokens.begin(), configTokens.end());
            for (const auto& qToken : queryTokens) {
                if (configTokenSet.find(qToken) != configTokenSet.end()) {
                    matchedTokens++;
                }
            }
            if (matchedTokens == static_cast<int>(queryTokens.size())) {
                multiTokenBoost = 1.0f;  // All query tokens present
            }
        }
        
        // 4. Combined scoring with token matching priority
        // Token matching (exact) > Semantic similarity (fuzzy)
        float tokenScore = tokenOverlap + multiTokenBoost;
        float combinedScore = 0.5f * tokenScore + 0.5f * semanticScore;
        
        // v1.5: Apply search interest bias (gentle clamped boost)
        // Config tokens that match user's past interests get boosted
        float configBias = userContext_.getSearchTracker().getBiasSignal(configTokens);
        combinedScore += configBias;  // Additive bias (already clamped in tracker)
        
        // Apply user boost
        combinedScore *= userContext_.calculateUserBoost(configId);
        
        if (combinedScore > 0.05f) {  // Lower threshold for token matches
            results.emplace_back(config, combinedScore);
        }
    }
    
    // v1.4: Re-rank results with cosine-on-shared-tokens validation
    // This ensures semantic correctness after initial pointing
    for (auto& [config, score] : results) {
        auto configTokens = config->getAllTokens();
        
        // Calculate shared token count
        int sharedTokens = 0;
        std::unordered_set<std::string> configTokenSet(configTokens.begin(), configTokens.end());
        for (const auto& qToken : queryTokens) {
            if (configTokenSet.find(qToken) != configTokenSet.end()) {
                sharedTokens++;
            }
        }
        
        // If shared tokens > threshold, validate with semantic similarity
        if (sharedTokens > 0) {
            float semanticValidation = EmbeddingEngine::calculateSimilarity(
                queryEmbedding, config->getEmbedding());
            
            // Boost if both token match AND semantic match are strong
            if (semanticValidation > 0.3f) {
                score *= (1.0f + 0.2f * semanticValidation);
            }
        }
    }
    
    // Sort by score descending
    std::sort(results.begin(), results.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Limit results
    if (results.size() > static_cast<size_t>(maxResults)) {
        results.resize(maxResults);
    }
    
    return results;
}

std::shared_ptr<AudioConfig> AudioConfigSystem::getConfiguration(const ConfigId& configId) const {
    auto it = configurations_.find(configId);
    return it != configurations_.end() ? it->second : nullptr;
}

bool AudioConfigSystem::generateSynthesisConfiguration(const std::string& outputPath) const {
    try {
        std::vector<std::shared_ptr<AudioConfig>> selectedConfigs;
        
        for (const auto& configId : userContext_.getSelectedConfigs()) {
            auto config = getConfiguration(configId);
            if (config) {
                selectedConfigs.push_back(config);
            }
        }
        
        if (selectedConfigs.empty()) {
            std::cout << "No configurations selected. Use 'select <config_id>' to select configurations." << std::endl;
            return false;
        }
        
        auto synthesisConfig = ConfigGenerator::generateSynthesisConfig(selectedConfigs, userContext_);
        
        std::ofstream outputFile(outputPath);
        if (!outputFile.is_open()) {
            std::cerr << "Could not create output file: " << outputPath << std::endl;
            return false;
        }
        
        outputFile << std::setw(2) << *synthesisConfig << std::endl;
        outputFile.close();
        
        std::cout << "Generated synthesis configuration: " << outputPath << std::endl;
        std::cout << "Contains " << selectedConfigs.size() << " instruments with full compatibility analysis" << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Config generation failed: " << e.what() << std::endl;
        return false;
    }
}

// ============================================================================
// CLI Implementation
// ============================================================================

void AudioConfigSystem::runInteractiveCLI() {
    std::string input;
    
    std::cout << "\n=== INTERACTIVE SESSION ===" << std::endl;
    std::cout << "Commands: search, select, boost, demote, exclude, list, stats, signals, generate, help, examples, quit\n" << std::endl;
    
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);
        
        if (input.empty()) continue;
        
        auto tokens = tokenizeCommand(input);
        if (tokens.empty()) continue;
        
        std::string command = tokens[0];
        std::transform(command.begin(), command.end(), command.begin(), ::tolower);
        
        try {
            if (command == "quit" || command == "exit") {
                break;
            } else if (command == "help") {
                handleHelpCommand(tokens);
            } else if (command == "examples") {
                handleExamplesCommand(tokens);
            } else if (command == "search") {
                handleSearchCommand(tokens);
            } else if (command == "select") {
                handleSelectCommand(tokens);
            } else if (command == "boost") {
                handleBoostCommand(tokens);
            } else if (command == "demote") {
                handleDemoteCommand(tokens);
            } else if (command == "exclude") {
                handleExcludeCommand(tokens);
            } else if (command == "list") {
                handleListCommand(tokens);
            } else if (command == "stats") {
                handleStatsCommand(tokens);
            } else if (command == "signals") {
                handleSignalsCommand(tokens);
            } else if (command == "generate" || command == "suggest_config") {
                handleGenerateCommand(tokens);
            } else {
                std::cout << "Unknown command: " << command << std::endl;
                std::cout << "Type 'help' for available commands." << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Command error: " << e.what() << std::endl;
        }
    }
}

void AudioConfigSystem::handleSearchCommand(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "Usage: search <query>" << std::endl;
        std::cout << "Example: search warm aggressive" << std::endl;
        return;
    }
    
    std::string query;
    for (size_t i = 1; i < args.size(); ++i) {
        if (i > 1) query += " ";
        query += args[i];
    }
    
    std::cout << "\nSearching for: \"" << query << "\"" << std::endl;
    
    auto results = searchConfigurations(query, 10);
    
    if (results.empty()) {
        std::cout << "No matching configurations found." << std::endl;
        return;
    }
    
    std::cout << "Found " << results.size() << " matching configurations:\n" << std::endl;
    
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& [config, score] = results[i];
        std::cout << (i + 1) << ". ";
        printConfigurationSummary(*config, score);
        std::cout << std::endl;
    }
    
    std::cout << "\nUse 'select <config_id>' to add to your selection" << std::endl;
    std::cout << "Use 'boost <config_id>' if you like a result" << std::endl;
}

void AudioConfigSystem::handleSelectCommand(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        std::cout << "Usage: select <config_id>" << std::endl;
        return;
    }
    
    const std::string& configId = args[1];
    auto config = getConfiguration(configId);
    
    if (!config) {
        std::cout << "Configuration not found: " << configId << std::endl;
        return;
    }
    
    userContext_.selectConfig(configId);
    std::cout << "Selected: " << configId << std::endl;
    
    // Show compatibility with existing selections
    const auto& selectedConfigs = userContext_.getSelectedConfigs();
    if (selectedConfigs.size() > 1) {
        std::cout << "\nCompatibility with existing selections:" << std::endl;
        
        for (const auto& otherConfigId : selectedConfigs) {
            if (otherConfigId == configId) continue;
            
            auto otherConfig = getConfiguration(otherConfigId);
            if (!otherConfig) continue;
            
            auto compatibility = pointer_->analyzeCompatibility(*config, *otherConfig);
            
            std::cout << "  " << otherConfigId << ": " 
                      << std::fixed << std::setprecision(2) << compatibility.overallScore
                      << (compatibility.isRecommended ? " (recommended)" : " (warning)") << std::endl;
        }
    }
}

void AudioConfigSystem::handleBoostCommand(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        std::cout << "Usage: boost <config_id>" << std::endl;
        return;
    }
    
    const std::string& configId = args[1];
    auto config = getConfiguration(configId);
    
    if (!config) {
        std::cout << "Configuration not found: " << configId << std::endl;
        return;
    }
    
    userContext_.recordPositiveChoice(configId);
    std::cout << "Boosted: " << configId << " (future searches will prefer similar configurations)" << std::endl;
}

void AudioConfigSystem::handleDemoteCommand(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        std::cout << "Usage: demote <config_id>" << std::endl;
        return;
    }
    
    const std::string& configId = args[1];
    auto config = getConfiguration(configId);
    
    if (!config) {
        std::cout << "Configuration not found: " << configId << std::endl;
        return;
    }
    
    userContext_.recordNegativeChoice(configId);
    std::cout << "Demoted: " << configId << " (future searches will avoid similar configurations)" << std::endl;
}

void AudioConfigSystem::handleExcludeCommand(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        std::cout << "Usage: exclude <config_id>" << std::endl;
        return;
    }
    
    const std::string& configId = args[1];
    userContext_.excludeConfig(configId);
    std::cout << "Excluded: " << configId << " (will not appear in future searches)" << std::endl;
}

void AudioConfigSystem::handleListCommand(const std::vector<std::string>& args) {
    (void)args; // Unused parameter
    const auto& selectedConfigs = userContext_.getSelectedConfigs();
    
    if (selectedConfigs.empty()) {
        std::cout << "No configurations currently selected." << std::endl;
        std::cout << "Use 'search <query>' to find configurations and 'select <config_id>' to add them." << std::endl;
        return;
    }
    
    std::cout << "\nSelected Configurations (" << selectedConfigs.size() << "):" << std::endl;
    
    for (size_t i = 0; i < selectedConfigs.size(); ++i) {
        const auto& configId = selectedConfigs[i];
        auto config = getConfiguration(configId);
        
        if (config) {
            std::cout << (i + 1) << ". ";
            printConfigurationSummary(*config);
            std::cout << std::endl;
        }
    }
    
    std::cout << "\nUse 'generate output.json' to create synthesis configuration" << std::endl;
}

void AudioConfigSystem::handleStatsCommand(const std::vector<std::string>& args) {
    (void)args; // Unused parameter
    std::cout << "\n=== SYSTEM STATISTICS ===" << std::endl;
    std::cout << "Total configurations: " << configurations_.size() << std::endl;
    std::cout << "Selected configurations: " << userContext_.getSelectedConfigs().size() << std::endl;
    
    // Count by musical role
    std::map<MusicalRole, int> roleCounts;
    for (const auto& [configId, config] : configurations_) {
        roleCounts[config->getMusicalRole().primaryRole]++;
    }
    
    std::cout << "\nBy musical role:" << std::endl;
    for (const auto& [role, count] : roleCounts) {
        std::string roleName;
        switch (role) {
            case MusicalRole::Lead: roleName = "lead"; break;
            case MusicalRole::Bass: roleName = "bass"; break;
            case MusicalRole::Pad: roleName = "pad"; break;
            case MusicalRole::Arp: roleName = "arp"; break;
            case MusicalRole::Percussion: roleName = "percussion"; break;
            case MusicalRole::Chord: roleName = "chord"; break;
            case MusicalRole::FX: roleName = "fx"; break;
            default: roleName = "unknown"; break;
        }
        std::cout << "  " << roleName << ": " << count << std::endl;
    }
    std::cout << "=========================================================" << std::endl;
}

void AudioConfigSystem::handleGenerateCommand(const std::vector<std::string>& args) {
    std::string outputPath = "generated_config.json";
    if (args.size() > 1) {
        outputPath = args[1];
    }
    
    std::cout << "\nGenerating synthesis configuration..." << std::endl;
    
    if (generateSynthesisConfiguration(outputPath)) {
        std::cout << "Configuration generated successfully!" << std::endl;
        
        // Validate the generated configuration
        std::vector<std::shared_ptr<AudioConfig>> selectedConfigs;
        for (const auto& configId : userContext_.getSelectedConfigs()) {
            auto config = getConfiguration(configId);
            if (config) {
                selectedConfigs.push_back(config);
            }
        }
        
        auto validation = ConfigGenerator::validateConfigChain(selectedConfigs);
        std::cout << "\nConfiguration Validation:" << std::endl;
        printCompatibilityResult(validation);
    }
}

void AudioConfigSystem::handleHelpCommand(const std::vector<std::string>& args) {
    (void)args; // Unused parameter
    std::cout << R"(
Multi-Dimensional Audio Configuration System - Help
====================================================

Available Commands:

SEARCH & DISCOVERY:
  search <query>          - Search configurations by semantic similarity
                           Example: search warm aggressive guitar
  
SELECTION & MANAGEMENT:
  select <config_id>      - Add configuration to your selection
  list                    - Show selected configurations
  
LEARNING & PREFERENCES:
  boost <config_id>       - Mark as preferred (improves future suggestions)
  demote <config_id>      - Mark as disliked (reduces future suggestions)
  exclude <config_id>     - Exclude from all future searches
  
GENERATION & OUTPUT:
  generate [filename]     - Generate synthesis-ready configuration
  suggest_config [file]   - Alias for generate command
  
INFORMATION:
  stats                   - Show system statistics and user preferences
  help                    - Show this help message
  examples                - Show usage examples and patterns
  
EXIT:
  quit / exit             - Exit the application

Tips:
  - Use semantic terms: "warm", "aggressive", "bright", "calm"
  - Musical roles: "lead", "bass", "pad", "arp", "chord"
  - Technical terms: "attack", "reverb", "filter", "envelope"
  - Combine multiple terms for better results
)" << std::endl;
}

void AudioConfigSystem::handleExamplesCommand(const std::vector<std::string>& args) {
    (void)args; // Unused parameter
    std::cout << R"(
Usage Examples & Patterns
=========================

SEMANTIC SEARCH EXAMPLES:
  search warm guitar          - Find warm-sounding guitar configurations
  search aggressive bass      - Find aggressive bass sounds
  search bright lead          - Find bright lead instruments
  search calm pad reverb      - Find calming pad sounds with reverb
  search vintage analog       - Find vintage-style analog instruments

WORKFLOW EXAMPLES:

1. Building a Lead + Bass + Pad combination:
   search lead bright
   select Lead_Bright_Energetic
   search bass punchy
   select Bass_Classic_MoogPunch  
   search pad warm
   select Pad_Warm_Calm
   generate my_track.json

2. Exploring and refining results:
   search guitar acoustic
   boost Acoustic_Warm_Fingerstyle    # I like this one
   demote Classical_Nylon_Soft        # Not what I want
   search guitar acoustic             # Re-search with updated preferences

3. Building genre-specific configurations:
   search electronic aggressive       # For electronic music
   search jazz warm smooth           # For jazz arrangements  
   search ambient calm ethereal      # For ambient textures

MULTI-DIMENSIONAL MATCHING:
The system considers 4 dimensions simultaneously:
  - Semantic: Term similarity and embeddings
  - Technical: Sample rates, plugin formats, compatibility
  - Musical Role: Lead/bass/pad function and typical combinations  
  - Layering: Frequency ranges, stereo placement, arrangement

ITERATIVE REFINEMENT:
  search warm                    # Initial broad search
  boost Pad_Warm_Calm           # Learn preferences
  exclude Bass_DigitalGrowl     # Remove unwanted results
  search warm                   # Refined results based on learning

SCORING BREAKDOWN:
Each suggestion shows:
  - Overall compatibility score (0.0-1.0)
  - Individual dimension scores
  - Specific reasons for compatibility
  - Warnings about potential conflicts
  - Suggestions for improvements
)" << std::endl;
}

void AudioConfigSystem::handleSignalsCommand(const std::vector<std::string>& args) {
    // v1.5: Comprehensive signals CLI command family
    
    if (args.size() < 2) {
        std::cout << "Usage: signals <subcommand> [options]\n\n";
        std::cout << "Subcommands:\n";
        std::cout << "  on          - Enable search interest tracking\n";
        std::cout << "  off         - Disable search interest tracking\n";
        std::cout << "  status      - Show tracking status and statistics\n";
        std::cout << "  history     - Show recent search queries\n";
        std::cout << "  active      - Show active interest signals\n";
        std::cout << "  tune        - Adjust tracking parameters\n";
        std::cout << "  export      - Export tracker state to file\n";
        std::cout << "  import      - Import tracker state from file\n";
        std::cout << "  clear       - Clear all tracked data\n";
        std::cout << "  help        - Show detailed help\n";
        return;
    }
    
    std::string subcommand = args[1];
    std::transform(subcommand.begin(), subcommand.end(), subcommand.begin(), ::tolower);
    
    auto& tracker = userContext_.getSearchTracker();
    
    if (subcommand == "on") {
        tracker.setEnabled(true);
        std::cout << "Search interest tracking: ENABLED" << std::endl;
        std::cout << "Your searches will now be tracked to improve future recommendations." << std::endl;
        
    } else if (subcommand == "off") {
        tracker.setEnabled(false);
        std::cout << "Search interest tracking: DISABLED" << std::endl;
        std::cout << "Your searches will no longer be tracked (existing data preserved)." << std::endl;
        
    } else if (subcommand == "status") {
        auto stats = tracker.getStatistics();
        auto params = tracker.getParameters();
        
        std::cout << "\n=== SEARCH INTEREST TRACKER STATUS ===" << std::endl;
        std::cout << "Tracking: " << (tracker.isEnabled() ? "ENABLED" : "DISABLED") << std::endl;
        std::cout << "\nStatistics:" << std::endl;
        std::cout << "  Total queries tracked: " << static_cast<int>(stats["total_queries"]) << std::endl;
        std::cout << "  Active signals: " << static_cast<int>(stats["active_signals"]) << std::endl;
        std::cout << "  Total tokens tracked: " << static_cast<int>(stats["total_tokens_tracked"]) << std::endl;
        std::cout << "  Average signal strength: " << std::fixed << std::setprecision(3) 
                  << stats["avg_signal_strength"] << std::endl;
        
        std::cout << "\nParameters:" << std::endl;
        std::cout << "  Decay half-life: " << params.decayHalfLife << " seconds (" 
                  << (params.decayHalfLife / 3600.0f) << " hours)" << std::endl;
        std::cout << "  Smoothing alpha: " << params.smoothingAlpha << std::endl;
        std::cout << "  Bias strength: " << params.biasStrength << std::endl;
        std::cout << "  Bias clamp max: " << params.biasClampMax << std::endl;
        std::cout << "  Min signal strength: " << params.minSignalStrength << std::endl;
        
    } else if (subcommand == "history") {
        int maxResults = 20;
        if (args.size() > 2) {
            try {
                maxResults = std::stoi(args[2]);
            } catch (...) {}
        }
        
        auto history = tracker.getHistory(maxResults);
        
        std::cout << "\n=== SEARCH HISTORY (Most Recent " << history.size() << ") ===" << std::endl;
        
        if (history.empty()) {
            std::cout << "No search history available." << std::endl;
        } else {
            for (size_t i = 0; i < history.size(); ++i) {
                const auto& record = history[i];
                auto now = std::chrono::system_clock::now();
                float ageSeconds = record.getAgeSeconds(now);
                float decayedStrength = record.getDecayedStrength(now, tracker.getParameters().decayHalfLife);
                
                std::cout << (i + 1) << ". ";
                std::cout << "[" << TextUtils::joinTokens(record.tokens) << "] ";
                std::cout << "(" << static_cast<int>(ageSeconds / 60) << " min ago, ";
                std::cout << "strength: " << std::fixed << std::setprecision(2) << decayedStrength << ")" << std::endl;
            }
        }
        
    } else if (subcommand == "active") {
        auto activeSignals = tracker.getActiveSignals();
        
        std::cout << "\n=== ACTIVE INTEREST SIGNALS (" << activeSignals.size() << ") ===" << std::endl;
        
        if (activeSignals.empty()) {
            std::cout << "No active signals." << std::endl;
        } else {
            // Sort by strength descending
            std::vector<std::pair<std::string, float>> sortedSignals(activeSignals.begin(), activeSignals.end());
            std::sort(sortedSignals.begin(), sortedSignals.end(),
                     [](const auto& a, const auto& b) { return a.second > b.second; });
            
            for (const auto& [token, strength] : sortedSignals) {
                std::cout << "  " << token << ": " << std::fixed << std::setprecision(3) << strength;
                
                // Show visual bar
                int barLength = static_cast<int>(strength * 20);
                std::cout << " [";
                for (int i = 0; i < 20; ++i) {
                    std::cout << (i < barLength ? "=" : " ");
                }
                std::cout << "]" << std::endl;
            }
        }
        
    } else if (subcommand == "tune") {
        if (args.size() < 4) {
            std::cout << "Usage: signals tune <parameter> <value>\n\n";
            std::cout << "Parameters:\n";
            std::cout << "  decay_halflife    - Decay half-life in seconds (default: 7200)\n";
            std::cout << "  smoothing_alpha   - EMA smoothing factor 0-1 (default: 0.3)\n";
            std::cout << "  bias_strength     - Bias multiplier 0-1 (default: 0.2)\n";
            std::cout << "  bias_clamp        - Maximum bias contribution (default: 0.15)\n";
            std::cout << "  min_signal        - Minimum signal to keep (default: 0.01)\n";
            return;
        }
        
        std::string param = args[2];
        float value;
        try {
            value = std::stof(args[3]);
        } catch (...) {
            std::cout << "Error: Invalid value" << std::endl;
            return;
        }
        
        auto params = tracker.getParameters();
        
        if (param == "decay_halflife") {
            params.decayHalfLife = value;
        } else if (param == "smoothing_alpha") {
            params.smoothingAlpha = value;
        } else if (param == "bias_strength") {
            params.biasStrength = value;
        } else if (param == "bias_clamp") {
            params.biasClampMax = value;
        } else if (param == "min_signal") {
            params.minSignalStrength = value;
        } else {
            std::cout << "Error: Unknown parameter '" << param << "'" << std::endl;
            return;
        }
        
        if (!params.isValid()) {
            std::cout << "Error: Invalid parameter value (out of range)" << std::endl;
            return;
        }
        
        tracker.setParameters(params);
        std::cout << "Parameter updated: " << param << " = " << value << std::endl;
        
    } else if (subcommand == "export") {
        std::string filename = "signals_state.json";
        if (args.size() > 2) {
            filename = args[2];
        }
        
        auto state = tracker.exportState();
        
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cout << "Error: Could not open file '" << filename << "' for writing" << std::endl;
            return;
        }
        
        file << state.dump(2);  // Pretty print with 2-space indent
        file.close();
        
        std::cout << "Tracker state exported to: " << filename << std::endl;
        std::cout << "  Queries: " << tracker.getHistory(1000).size() << std::endl;
        std::cout << "  Active signals: " << tracker.getActiveSignals().size() << std::endl;
        
    } else if (subcommand == "import") {
        if (args.size() < 3) {
            std::cout << "Usage: signals import <filename>" << std::endl;
            return;
        }
        
        std::string filename = args[2];
        
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cout << "Error: Could not open file '" << filename << "'" << std::endl;
            return;
        }
        
        nlohmann::json state;
        try {
            file >> state;
        } catch (const std::exception& e) {
            std::cout << "Error: Invalid JSON file - " << e.what() << std::endl;
            return;
        }
        
        if (tracker.importState(state)) {
            std::cout << "Tracker state imported from: " << filename << std::endl;
            auto stats = tracker.getStatistics();
            std::cout << "  Queries: " << static_cast<int>(stats["total_queries"]) << std::endl;
            std::cout << "  Active signals: " << static_cast<int>(stats["active_signals"]) << std::endl;
        } else {
            std::cout << "Error: Failed to import state (invalid format)" << std::endl;
        }
        
    } else if (subcommand == "clear") {
        tracker.clear();
        std::cout << "All tracked data cleared." << std::endl;
        
    } else if (subcommand == "help") {
        std::cout << R"(
=== SIGNALS COMMAND HELP ===

The signals system tracks your search interests over time with temporal decay
and uses them to gently bias future search results toward your preferences.

SUBCOMMANDS:

  signals on/off
    Enable or disable search interest tracking
    
  signals status
    Show current tracking status, statistics, and parameters
    
  signals history [n]
    Show recent search queries (default: 20, max: all)
    Shows query tokens, age, and decayed strength
    
  signals active
    Show currently active interest signals
    Lists tokens with their decayed strengths and visual bars
    
  signals tune <parameter> <value>
    Adjust tracking parameters:
    - decay_halflife: How fast interests fade (seconds, default: 7200)
    - smoothing_alpha: EMA smoothing factor (0-1, default: 0.3)
    - bias_strength: How much to bias search (0-1, default: 0.2)
    - bias_clamp: Maximum bias contribution (0-1, default: 0.15)
    - min_signal: Minimum signal to keep (default: 0.01)
    
  signals export [filename]
    Export tracker state to JSON file (default: signals_state.json)
    Includes parameters, active signals, and query history
    
  signals import <filename>
    Import tracker state from JSON file
    Replaces current tracker state
    
  signals clear
    Clear all tracked data (history and signals)
    Does not disable tracking

HOW IT WORKS:

1. Every search query is tokenized and recorded with a timestamp
2. Token signals decay exponentially over time (half-life: 2 hours default)
3. New queries are smoothed with existing signals using EMA (alpha: 0.3)
4. During search, matching tokens provide a gentle bias boost (0-0.15 max)
5. Weak signals below threshold are pruned automatically

EXAMPLES:

  signals on                          # Enable tracking
  signals history 50                  # Show last 50 queries
  signals tune decay_halflife 3600    # Set 1-hour decay
  signals export my_interests.json    # Save state
  signals import my_interests.json    # Load state
  signals clear                       # Reset all data

)" << std::endl;
        
    } else {
        std::cout << "Unknown subcommand: " << subcommand << std::endl;
        std::cout << "Use 'signals help' for detailed information" << std::endl;
    }
}

void AudioConfigSystem::printConfigurationSummary(const AudioConfig& config, CompatibilityScore score) const {
    std::cout << config.getName();
    
    if (score >= 0.0f) {
        std::cout << " (Score: " << std::fixed << std::setprecision(2) << score << ")";
    }
    
    // Show musical role
    const auto& role = config.getMusicalRole();
    std::string roleName;
    switch (role.primaryRole) {
        case MusicalRole::Lead: roleName = "Lead"; break;
        case MusicalRole::Bass: roleName = "Bass"; break;
        case MusicalRole::Pad: roleName = "Pad"; break;
        case MusicalRole::Arp: roleName = "Arp"; break;
        case MusicalRole::Percussion: roleName = "Percussion"; break;
        case MusicalRole::Chord: roleName = "Chord"; break;
        case MusicalRole::FX: roleName = "FX"; break;
        default: roleName = "Unknown"; break;
    }
    
    std::cout << " [" << roleName;
    if (role.tonalCharacter != "neutral") {
        std::cout << ", " << role.tonalCharacter;
    }
    std::cout << "]";
    
    // Show semantic tags
    const auto& tags = config.getSemanticTags();
    if (!tags.empty()) {
        std::cout << " Tags: ";
        for (size_t i = 0; i < std::min(tags.size(), size_t(3)); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << tags[i];
        }
        if (tags.size() > 3) {
            std::cout << "...";
        }
    }
}

void AudioConfigSystem::printCompatibilityResult(const CompatibilityResult& result) const {
    std::cout << result.generateExplanation() << std::endl;
}

std::vector<std::string> AudioConfigSystem::tokenizeCommand(const std::string& command) const {
    std::vector<std::string> tokens;
    std::istringstream iss(command);
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

} // namespace audio_config