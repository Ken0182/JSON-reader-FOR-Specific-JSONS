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

bool AudioConfigSystem::initialize(const std::string& configDatabasePath) {
    try {
        loadConfigurationDatabase(configDatabasePath);
        
        std::cout << "✅ Initialized Multi-Dimensional Audio Configuration System" << std::endl;
        std::cout << "📊 Loaded " << configurations_.size() << " configurations" << std::endl;
        std::cout << "⚙️  Scoring weights: Semantic(" << weights_.semantic 
                  << ") Technical(" << weights_.technical 
                  << ") Role(" << weights_.musicalRole 
                  << ") Layering(" << weights_.layering << ")" << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "❌ Initialization failed: " << e.what() << std::endl;
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
        
        // Generate embedding
        std::string embeddingText = configId;
        for (const auto& tag : audioConfig->getSemanticTags()) {
            embeddingText += " " + tag;
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
    
    // Generate query embedding
    EmbeddingVector queryEmbedding = embeddingEngine_->getEmbedding(query);
    
    for (const auto& [configId, config] : configurations_) {
        // Skip excluded configurations
        if (userContext_.isExcluded(configId)) continue;
        
        // Calculate semantic similarity
        float semanticScore = EmbeddingEngine::calculateSimilarity(queryEmbedding, config->getEmbedding());
        
        // Check for direct text matches
        float textScore = 0.0f;
        std::string queryLower = query;
        std::string configIdLower = configId;
        std::transform(queryLower.begin(), queryLower.end(), queryLower.begin(), ::tolower);
        std::transform(configIdLower.begin(), configIdLower.end(), configIdLower.begin(), ::tolower);
        
        if (configIdLower.find(queryLower) != std::string::npos) {
            textScore += 1.0f;
        }
        
        // Check tag matches
        for (const auto& tag : config->getSemanticTags()) {
            std::string tagLower = tag;
            std::transform(tagLower.begin(), tagLower.end(), tagLower.begin(), ::tolower);
            if (tagLower.find(queryLower) != std::string::npos) {
                textScore += 0.8f;
            }
        }
        
        // Combined score
        float combinedScore = 0.4f * textScore + 0.6f * semanticScore;
        
        // Apply user boost
        combinedScore *= userContext_.calculateUserBoost(configId);
        
        if (combinedScore > 0.1f) {
            results.emplace_back(config, combinedScore);
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
            std::cout << "⚠️  No configurations selected. Use 'select <config_id>' to select configurations." << std::endl;
            return false;
        }
        
        auto synthesisConfig = ConfigGenerator::generateSynthesisConfig(selectedConfigs, userContext_);
        
        std::ofstream outputFile(outputPath);
        if (!outputFile.is_open()) {
            std::cerr << "❌ Could not create output file: " << outputPath << std::endl;
            return false;
        }
        
        outputFile << std::setw(2) << *synthesisConfig << std::endl;
        outputFile.close();
        
        std::cout << "🎵 Generated synthesis configuration: " << outputPath << std::endl;
        std::cout << "📊 Contains " << selectedConfigs.size() << " instruments with full compatibility analysis" << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "❌ Config generation failed: " << e.what() << std::endl;
        return false;
    }
}

// ============================================================================
// CLI Implementation
// ============================================================================

void AudioConfigSystem::runInteractiveCLI() {
    std::string input;
    
    std::cout << "\n🎹 Welcome to the Interactive CLI!" << std::endl;
    std::cout << "Type 'help' for available commands or 'examples' for usage patterns.\n" << std::endl;
    
    while (true) {
        std::cout << "🎵 > ";
        std::getline(std::cin, input);
        
        if (input.empty()) continue;
        
        auto tokens = tokenizeCommand(input);
        if (tokens.empty()) continue;
        
        std::string command = tokens[0];
        std::transform(command.begin(), command.end(), command.begin(), ::tolower);
        
        try {
            if (command == "quit" || command == "exit") {
                std::cout << "👋 Goodbye!" << std::endl;
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
            } else if (command == "generate" || command == "suggest_config") {
                handleGenerateCommand(tokens);
            } else {
                std::cout << "❓ Unknown command: " << command << std::endl;
                std::cout << "Type 'help' for available commands." << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "❌ Command error: " << e.what() << std::endl;
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
    
    std::cout << "\n🔍 Searching for: \"" << query << "\"" << std::endl;
    
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
    
    std::cout << "\n💡 Use 'select <config_id>' to add to your selection" << std::endl;
    std::cout << "💡 Use 'boost <config_id>' if you like a result" << std::endl;
}

void AudioConfigSystem::handleSelectCommand(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        std::cout << "Usage: select <config_id>" << std::endl;
        return;
    }
    
    const std::string& configId = args[1];
    auto config = getConfiguration(configId);
    
    if (!config) {
        std::cout << "❌ Configuration not found: " << configId << std::endl;
        return;
    }
    
    userContext_.selectConfig(configId);
    std::cout << "✅ Selected: " << configId << std::endl;
    
    // Show compatibility with existing selections
    const auto& selectedConfigs = userContext_.getSelectedConfigs();
    if (selectedConfigs.size() > 1) {
        std::cout << "\n🔗 Compatibility with existing selections:" << std::endl;
        
        for (const auto& otherConfigId : selectedConfigs) {
            if (otherConfigId == configId) continue;
            
            auto otherConfig = getConfiguration(otherConfigId);
            if (!otherConfig) continue;
            
            auto compatibility = pointer_->analyzeCompatibility(*config, *otherConfig);
            
            std::cout << "  • " << otherConfigId << ": " 
                      << std::fixed << std::setprecision(2) << compatibility.overallScore
                      << (compatibility.isRecommended ? " ✅" : " ⚠️") << std::endl;
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
        std::cout << "❌ Configuration not found: " << configId << std::endl;
        return;
    }
    
    userContext_.recordPositiveChoice(configId);
    std::cout << "👍 Boosted: " << configId << " (future searches will prefer similar configurations)" << std::endl;
}

void AudioConfigSystem::handleDemoteCommand(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        std::cout << "Usage: demote <config_id>" << std::endl;
        return;
    }
    
    const std::string& configId = args[1];
    auto config = getConfiguration(configId);
    
    if (!config) {
        std::cout << "❌ Configuration not found: " << configId << std::endl;
        return;
    }
    
    userContext_.recordNegativeChoice(configId);
    std::cout << "👎 Demoted: " << configId << " (future searches will avoid similar configurations)" << std::endl;
}

void AudioConfigSystem::handleExcludeCommand(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        std::cout << "Usage: exclude <config_id>" << std::endl;
        return;
    }
    
    const std::string& configId = args[1];
    userContext_.excludeConfig(configId);
    std::cout << "🚫 Excluded: " << configId << " (will not appear in future searches)" << std::endl;
}

void AudioConfigSystem::handleListCommand(const std::vector<std::string>& args) {
    const auto& selectedConfigs = userContext_.getSelectedConfigs();
    
    if (selectedConfigs.empty()) {
        std::cout << "No configurations currently selected." << std::endl;
        std::cout << "Use 'search <query>' to find configurations and 'select <config_id>' to add them." << std::endl;
        return;
    }
    
    std::cout << "\n📋 Selected Configurations (" << selectedConfigs.size() << "):" << std::endl;
    
    for (size_t i = 0; i < selectedConfigs.size(); ++i) {
        const auto& configId = selectedConfigs[i];
        auto config = getConfiguration(configId);
        
        if (config) {
            std::cout << (i + 1) << ". ";
            printConfigurationSummary(*config);
            std::cout << std::endl;
        }
    }
    
    std::cout << "\n💡 Use 'generate output.json' to create synthesis configuration" << std::endl;
}

void AudioConfigSystem::handleStatsCommand(const std::vector<std::string>& args) {
    std::cout << "\n📊 System Statistics:" << std::endl;
    std::cout << "  • Total configurations: " << configurations_.size() << std::endl;
    std::cout << "  • Selected configurations: " << userContext_.getSelectedConfigs().size() << std::endl;
    
    // Count by musical role
    std::map<MusicalRole, int> roleCounts;
    for (const auto& [configId, config] : configurations_) {
        roleCounts[config->getMusicalRole().primaryRole]++;
    }
    
    std::cout << "\n🎼 By Musical Role:" << std::endl;
    for (const auto& [role, count] : roleCounts) {
        std::string roleName;
        switch (role) {
            case MusicalRole::Lead: roleName = "Lead"; break;
            case MusicalRole::Bass: roleName = "Bass"; break;
            case MusicalRole::Pad: roleName = "Pad"; break;
            case MusicalRole::Arp: roleName = "Arp"; break;
            case MusicalRole::Percussion: roleName = "Percussion"; break;
            case MusicalRole::Chord: roleName = "Chord"; break;
            case MusicalRole::FX: roleName = "FX"; break;
            default: roleName = "Unknown"; break;
        }
        std::cout << "  • " << roleName << ": " << count << std::endl;
    }
    
    std::cout << "\n⚙️  Scoring Weights:" << std::endl;
    std::cout << "  • Semantic: " << weights_.semantic << std::endl;
    std::cout << "  • Technical: " << weights_.technical << std::endl;
    std::cout << "  • Musical Role: " << weights_.musicalRole << std::endl;
    std::cout << "  • Layering: " << weights_.layering << std::endl;
}

void AudioConfigSystem::handleGenerateCommand(const std::vector<std::string>& args) {
    std::string outputPath = "generated_config.json";
    if (args.size() > 1) {
        outputPath = args[1];
    }
    
    std::cout << "\n🎵 Generating synthesis configuration..." << std::endl;
    
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
        std::cout << "\n🔍 Configuration Validation:" << std::endl;
        printCompatibilityResult(validation);
    }
}

void AudioConfigSystem::handleHelpCommand(const std::vector<std::string>& args) {
    std::cout << R"(
🎹 Multi-Dimensional Audio Configuration System - Help

📖 Available Commands:

🔍 SEARCH & DISCOVERY:
  search <query>          - Search configurations by semantic similarity
                           Example: search warm aggressive guitar
  
📋 SELECTION & MANAGEMENT:
  select <config_id>      - Add configuration to your selection
  list                    - Show selected configurations
  
🎯 LEARNING & PREFERENCES:
  boost <config_id>       - Mark as preferred (improves future suggestions)
  demote <config_id>      - Mark as disliked (reduces future suggestions)
  exclude <config_id>     - Exclude from all future searches
  
⚙️  GENERATION & OUTPUT:
  generate [filename]     - Generate synthesis-ready configuration
  suggest_config [file]   - Alias for generate command
  
📊 INFORMATION:
  stats                   - Show system statistics and user preferences
  help                    - Show this help message
  examples                - Show usage examples and patterns
  
🚪 EXIT:
  quit / exit             - Exit the application

💡 Tips:
  • Use semantic terms: "warm", "aggressive", "bright", "calm"
  • Musical roles: "lead", "bass", "pad", "arp", "chord"
  • Technical terms: "attack", "reverb", "filter", "envelope"
  • Combine multiple terms for better results
)" << std::endl;
}

void AudioConfigSystem::handleExamplesCommand(const std::vector<std::string>& args) {
    std::cout << R"(
🎯 Usage Examples & Patterns:

🔍 SEMANTIC SEARCH EXAMPLES:
  search warm guitar          - Find warm-sounding guitar configurations
  search aggressive bass      - Find aggressive bass sounds
  search bright lead          - Find bright lead instruments
  search calm pad reverb      - Find calming pad sounds with reverb
  search vintage analog       - Find vintage-style analog instruments

🎼 WORKFLOW EXAMPLES:

1️⃣ Building a Lead + Bass + Pad combination:
   search lead bright
   select Lead_Bright_Energetic
   search bass punchy
   select Bass_Classic_MoogPunch  
   search pad warm
   select Pad_Warm_Calm
   generate my_track.json

2️⃣ Exploring and refining results:
   search guitar acoustic
   boost Acoustic_Warm_Fingerstyle    # I like this one
   demote Classical_Nylon_Soft        # Not what I want
   search guitar acoustic             # Re-search with updated preferences

3️⃣ Building genre-specific configurations:
   search electronic aggressive       # For electronic music
   search jazz warm smooth           # For jazz arrangements  
   search ambient calm ethereal      # For ambient textures

🎯 MULTI-DIMENSIONAL MATCHING:
The system considers 4 dimensions simultaneously:
  • Semantic: Term similarity and embeddings
  • Technical: Sample rates, plugin formats, compatibility
  • Musical Role: Lead/bass/pad function and typical combinations  
  • Layering: Frequency ranges, stereo placement, arrangement

🔄 ITERATIVE REFINEMENT:
  search warm                    # Initial broad search
  boost Pad_Warm_Calm           # Learn preferences
  exclude Bass_DigitalGrowl     # Remove unwanted results
  search warm                   # Refined results based on learning

📊 SCORING BREAKDOWN:
Each suggestion shows:
  • Overall compatibility score (0.0-1.0)
  • Individual dimension scores
  • Specific reasons for compatibility
  • Warnings about potential conflicts
  • Suggestions for improvements
)" << std::endl;
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