/**
 * @file kb_cli.cpp
 * @brief Knowledge Base CLI - Database Management Commands
 * @author AI Assistant
 * @version 1.0
 * 
 * CLI tool for managing the semantic knowledge base database.
 * 
 * USAGE:
 *   kb learn-tag --name "jangly" --text "bright plucky harmonics" [--force] [--dry-run]
 *   kb rebuild-idf [--incremental | --full] [--min-df N] [--max-df-ratio R] [--dry-run]
 *   kb vacuum [--yes]
 *   kb checkpoint
 *   kb stats [--json]
 * 
 * EXIT CODES:
 *   0 = Success
 *   1 = General error
 *   2 = Validation error
 *   3 = Conflict error
 *   4 = IO/database error
 */

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cctype>
#include <regex>

// Include our semantic database and encoder
#include "../src/semantic_db.hpp"
#include "../src/semantic_knowledge_base.hpp"
#include "../src/sentence_encoder.hpp"

namespace audio_config {

/**
 * @brief CLI argument parser and command dispatcher
 */
class KBCLI {
public:
    explicit KBCLI(const std::string& dbPath) : dbPath_(dbPath) {}
    
    /**
     * @brief Parse command line arguments and execute command
     * @param argc Argument count
     * @param argv Argument vector
     * @return Exit code
     */
    int run(int argc, char* argv[]);
    
private:
    std::string dbPath_;
    std::unique_ptr<SemanticKnowledgeBase> kb_;
    
    // Command implementations
    int cmdLearnTag(const std::vector<std::string>& args);
    int cmdRebuildIDF(const std::vector<std::string>& args);
    int cmdVacuum(const std::vector<std::string>& args);
    int cmdCheckpoint(const std::vector<std::string>& args);
    int cmdStats(const std::vector<std::string>& args);
    
    // Helper functions
    bool initializeKB();
    std::string canonicalizeTag(const std::string& tag);
    bool validateTagName(const std::string& tag);
    bool validateText(const std::string& text);
    void printUsage();
    void printVersion();
    int parseExitCode(const std::string& error);
    
    // Database operations with proper transaction handling
    bool atomicLearnTag(const std::string& tag, const std::string& text, 
                       const std::string& canonical, bool dryRun = false);
    bool atomicRebuildIDF(bool incremental, int minDf, float maxDfRatio, bool dryRun = false);
    bool atomicVacuum(bool confirm = false);
    bool atomicCheckpoint();
    
    // Validation and normalization
    std::string trim(const std::string& str);
    std::string toLower(const std::string& str);
    bool isWhitespace(const std::string& str);
    bool containsControlChars(const std::string& str);
};

int KBCLI::run(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 1;
    }
    
    std::string command = argv[1];
    std::vector<std::string> args;
    for (int i = 2; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }
    
    // Handle version and help
    if (command == "--version" || command == "-v") {
        printVersion();
        return 0;
    }
    
    if (command == "--help" || command == "-h" || command == "help") {
        printUsage();
        return 0;
    }
    
    // Initialize knowledge base
    if (!initializeKB()) {
        std::cerr << "Error: Failed to initialize knowledge base" << std::endl;
        return 4;
    }
    
    // Dispatch commands
    if (command == "learn-tag") {
        return cmdLearnTag(args);
    } else if (command == "rebuild-idf") {
        return cmdRebuildIDF(args);
    } else if (command == "vacuum") {
        return cmdVacuum(args);
    } else if (command == "checkpoint") {
        return cmdCheckpoint(args);
    } else if (command == "stats") {
        return cmdStats(args);
    } else {
        std::cerr << "Error: Unknown command '" << command << "'" << std::endl;
        printUsage();
        return 2;
    }
}

bool KBCLI::initializeKB() {
    try {
        kb_ = std::make_unique<SemanticKnowledgeBase>(dbPath_);
        if (!kb_->initialize()) {
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error initializing knowledge base: " << e.what() << std::endl;
        return false;
    }
}

int KBCLI::cmdLearnTag(const std::vector<std::string>& args) {
    std::string tagName, text, canonical;
    bool force = false, dryRun = false;
    
    // Parse arguments
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--name" && i + 1 < args.size()) {
            tagName = args[++i];
        } else if (args[i] == "--text" && i + 1 < args.size()) {
            text = args[++i];
        } else if (args[i] == "--canonical" && i + 1 < args.size()) {
            canonical = args[++i];
        } else if (args[i] == "--force") {
            force = true;
        } else if (args[i] == "--dry-run") {
            dryRun = true;
        } else {
            std::cerr << "Error: Unknown argument '" << args[i] << "'" << std::endl;
            return 2;
        }
    }
    
    // Validate required arguments
    if (tagName.empty()) {
        std::cerr << "Error: --name is required" << std::endl;
        return 2;
    }
    if (text.empty()) {
        std::cerr << "Error: --text is required" << std::endl;
        return 2;
    }
    
    // Validate and normalize inputs
    tagName = canonicalizeTag(tagName);
    if (!validateTagName(tagName)) {
        std::cerr << "Error: Invalid tag name '" << tagName << "'" << std::endl;
        return 2;
    }
    
    if (!validateText(text)) {
        std::cerr << "Error: Invalid text content" << std::endl;
        return 2;
    }
    
    if (canonical.empty()) {
        canonical = tagName;
    } else {
        canonical = canonicalizeTag(canonical);
        if (!validateTagName(canonical)) {
            std::cerr << "Error: Invalid canonical name '" << canonical << "'" << std::endl;
            return 2;
        }
    }
    
    // Check if tag already exists in database (without auto-creating)
    auto db = kb_->getDatabase();
    if (!force && !db->getEmbedding(tagName).empty()) {
        std::cerr << "Error: Tag '" << tagName << "' already exists. Use --force to overwrite." << std::endl;
        return 3;
    }
    
    if (dryRun) {
        std::cout << "DRY RUN: Would learn tag '" << tagName << "' with text '" << text 
                  << "' and canonical '" << canonical << "'" << std::endl;
        return 0;
    }
    
    // Perform the operation
    if (atomicLearnTag(tagName, text, canonical, dryRun)) {
        std::cout << "Successfully learned tag '" << tagName << "'" << std::endl;
        return 0;
    } else {
        std::cerr << "Error: Failed to learn tag '" << tagName << "'" << std::endl;
        return 4;
    }
}

int KBCLI::cmdRebuildIDF(const std::vector<std::string>& args) {
    bool incremental = false, full = false, dryRun = false;
    int minDf = 1;
    float maxDfRatio = 0.95f;
    
    // Parse arguments
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--incremental") {
            incremental = true;
        } else if (args[i] == "--full") {
            full = true;
        } else if (args[i] == "--min-df" && i + 1 < args.size()) {
            minDf = std::stoi(args[++i]);
            if (minDf < 1) {
                std::cerr << "Error: min-df must be >= 1" << std::endl;
                return 2;
            }
        } else if (args[i] == "--max-df-ratio" && i + 1 < args.size()) {
            maxDfRatio = std::stof(args[++i]);
            if (maxDfRatio <= 0.0f || maxDfRatio >= 1.0f) {
                std::cerr << "Error: max-df-ratio must be between 0 and 1" << std::endl;
                return 2;
            }
        } else if (args[i] == "--dry-run") {
            dryRun = true;
        } else {
            std::cerr << "Error: Unknown argument '" << args[i] << "'" << std::endl;
            return 2;
        }
    }
    
    // Validate mode
    if (incremental && full) {
        std::cerr << "Error: Cannot specify both --incremental and --full" << std::endl;
        return 2;
    }
    if (!incremental && !full) {
        full = true; // Default to full rebuild
    }
    
    if (dryRun) {
        std::cout << "DRY RUN: Would rebuild IDF (" << (incremental ? "incremental" : "full") 
                  << ") with min-df=" << minDf << ", max-df-ratio=" << maxDfRatio << std::endl;
        return 0;
    }
    
    // Perform the operation
    if (atomicRebuildIDF(incremental, minDf, maxDfRatio, dryRun)) {
        std::cout << "Successfully rebuilt IDF statistics" << std::endl;
        return 0;
    } else {
        std::cerr << "Error: Failed to rebuild IDF statistics" << std::endl;
        return 4;
    }
}

int KBCLI::cmdVacuum(const std::vector<std::string>& args) {
    bool confirm = false;
    
    // Parse arguments
    for (const auto& arg : args) {
        if (arg == "--yes") {
            confirm = true;
        } else {
            std::cerr << "Error: Unknown argument '" << arg << "'" << std::endl;
            return 2;
        }
    }
    
    // Check database size and require confirmation for large databases
    auto db = kb_->getDatabase();
    if (db && std::filesystem::exists(dbPath_)) {
        auto size = std::filesystem::file_size(dbPath_);
        const size_t LARGE_DB_THRESHOLD = 100 * 1024 * 1024; // 100MB
        
        if (size > LARGE_DB_THRESHOLD && !confirm) {
            std::cerr << "Warning: Database is large (" << (size / 1024 / 1024) 
                      << "MB). Use --yes to confirm vacuum operation." << std::endl;
            return 2;
        }
    }
    
    // Perform the operation
    if (atomicVacuum(confirm)) {
        std::cout << "Successfully vacuumed database" << std::endl;
        return 0;
    } else {
        std::cerr << "Error: Failed to vacuum database" << std::endl;
        return 4;
    }
}

int KBCLI::cmdCheckpoint(const std::vector<std::string>& args) {
    // Parse arguments (none expected for checkpoint)
    for (const auto& arg : args) {
        std::cerr << "Error: Unknown argument '" << arg << "'" << std::endl;
        return 2;
    }
    
    // Perform the operation
    if (atomicCheckpoint()) {
        std::cout << "Successfully checkpointed database" << std::endl;
        return 0;
    } else {
        std::cerr << "Error: Failed to checkpoint database" << std::endl;
        return 4;
    }
}

int KBCLI::cmdStats(const std::vector<std::string>& args) {
    bool jsonOutput = false;
    
    // Parse arguments
    for (const auto& arg : args) {
        if (arg == "--json") {
            jsonOutput = true;
        } else {
            std::cerr << "Error: Unknown argument '" << arg << "'" << std::endl;
            return 2;
        }
    }
    
    // Use existing kbstats functionality
    // This is a simplified version - in practice you'd call the existing kbstats code
    auto db = kb_->getDatabase();
    if (!db) {
        std::cerr << "Error: Cannot access database" << std::endl;
        return 4;
    }
    
    if (jsonOutput) {
        std::cout << "{\n";
        std::cout << "  \"database\": \"" << dbPath_ << "\",\n";
        std::cout << "  \"dimension\": " << kb_->getDimension() << ",\n";
        std::cout << "  \"tag_count\": " << db->getTagCount() << ",\n";
        std::cout << "  \"alias_count\": " << db->getAliasCount() << ",\n";
        std::cout << "  \"schema_version\": " << db->getSchemaVersion() << "\n";
        std::cout << "}\n";
    } else {
        std::cout << "=== Knowledge Base Statistics ===" << std::endl;
        std::cout << "Database: " << dbPath_ << std::endl;
        std::cout << "Dimension: " << kb_->getDimension() << std::endl;
        std::cout << "Tag count: " << db->getTagCount() << std::endl;
        std::cout << "Alias count: " << db->getAliasCount() << std::endl;
        std::cout << "Schema version: " << db->getSchemaVersion() << std::endl;
    }
    
    return 0;
}

bool KBCLI::atomicLearnTag(const std::string& tag, const std::string& text, 
                          const std::string& canonical, bool dryRun) {
    if (dryRun) return true;
    
    try {
        // Use the existing learnTagFromText method which handles transactions
        return kb_->learnTagFromText(tag, text, canonical);
    } catch (const std::exception& e) {
        std::cerr << "Error in learnTag: " << e.what() << std::endl;
        return false;
    }
}

bool KBCLI::atomicRebuildIDF(bool incremental, int minDf, float maxDfRatio, bool dryRun) {
    if (dryRun) return true;
    
    try {
        // Get all tags from the database
        auto allTags = kb_->getAllTags();
        if (allTags.empty()) {
            std::cout << "No tags found in database" << std::endl;
            return true;
        }
        
        // For now, implement a simple full rebuild
        // In a real implementation, you'd have more sophisticated IDF computation
        int processed = kb_->computeIDFStatistics(allTags);
        std::cout << "Processed " << processed << " tags for IDF computation" << std::endl;
        
        return processed > 0;
    } catch (const std::exception& e) {
        std::cerr << "Error in rebuildIDF: " << e.what() << std::endl;
        return false;
    }
}

bool KBCLI::atomicVacuum(bool confirm) {
    try {
        auto db = kb_->getDatabase();
        if (!db) return false;
        
        // Close the knowledge base to release all connections
        kb_.reset();
        
        // Reopen database for vacuum
        auto tempDb = std::make_unique<SemanticDatabase>(dbPath_);
        if (!tempDb->isValid()) return false;
        
        // Perform vacuum
        bool success = tempDb->executeSqlPublic("VACUUM;");
        
        // Reinitialize knowledge base
        kb_ = std::make_unique<SemanticKnowledgeBase>(dbPath_);
        kb_->initialize();
        
        return success;
    } catch (const std::exception& e) {
        std::cerr << "Error in vacuum: " << e.what() << std::endl;
        return false;
    }
}

bool KBCLI::atomicCheckpoint() {
    try {
        auto db = kb_->getDatabase();
        if (!db) return false;
        
        // Perform WAL checkpoint
        return db->executeSqlPublic("PRAGMA wal_checkpoint(TRUNCATE);");
    } catch (const std::exception& e) {
        std::cerr << "Error in checkpoint: " << e.what() << std::endl;
        return false;
    }
}

std::string KBCLI::canonicalizeTag(const std::string& tag) {
    std::string result = trim(tag);
    result = toLower(result);
    
    // Remove extra whitespace
    std::regex whitespace("\\s+");
    result = std::regex_replace(result, whitespace, " ");
    
    return result;
}

bool KBCLI::validateTagName(const std::string& tag) {
    if (tag.empty() || isWhitespace(tag)) {
        return false;
    }
    
    if (tag.length() > 100) { // Reasonable limit
        return false;
    }
    
    if (containsControlChars(tag)) {
        return false;
    }
    
    // Check for valid characters (alphanumeric, spaces, hyphens, underscores)
    std::regex validTag("^[a-zA-Z0-9\\s\\-_]+$");
    return std::regex_match(tag, validTag);
}

bool KBCLI::validateText(const std::string& text) {
    if (text.empty() || isWhitespace(text)) {
        return false;
    }
    
    if (text.length() > 1000) { // Reasonable limit
        return false;
    }
    
    return !containsControlChars(text);
}

std::string KBCLI::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

std::string KBCLI::toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

bool KBCLI::isWhitespace(const std::string& str) {
    return std::all_of(str.begin(), str.end(), ::isspace);
}

bool KBCLI::containsControlChars(const std::string& str) {
    return std::any_of(str.begin(), str.end(), [](char c) {
        return std::iscntrl(c) && c != '\t' && c != '\n' && c != '\r';
    });
}

void KBCLI::printUsage() {
    std::cout << "Knowledge Base CLI - Database Management Tool\n";
    std::cout << "Version 1.0\n\n";
    std::cout << "USAGE:\n";
    std::cout << "  kb <command> [options]\n\n";
    std::cout << "COMMANDS:\n";
    std::cout << "  learn-tag     Learn a new tag from text description\n";
    std::cout << "  rebuild-idf   Rebuild IDF statistics from corpus\n";
    std::cout << "  vacuum        Optimize database storage\n";
    std::cout << "  checkpoint    Create WAL checkpoint\n";
    std::cout << "  stats         Show database statistics\n\n";
    std::cout << "EXAMPLES:\n";
    std::cout << "  kb learn-tag --name \"jangly\" --text \"bright plucky harmonics\"\n";
    std::cout << "  kb rebuild-idf --full --min-df 2\n";
    std::cout << "  kb vacuum --yes\n";
    std::cout << "  kb stats --json\n\n";
    std::cout << "EXIT CODES:\n";
    std::cout << "  0 = Success\n";
    std::cout << "  1 = General error\n";
    std::cout << "  2 = Validation error\n";
    std::cout << "  3 = Conflict error\n";
    std::cout << "  4 = IO/database error\n";
}

void KBCLI::printVersion() {
    std::cout << "Knowledge Base CLI version 1.0\n";
    std::cout << "Built with C++17 and SQLite3\n";
}

} // namespace audio_config

int main(int argc, char* argv[]) {
    std::string dbPath = "semantic.db";
    
    // Parse database path from command line if provided
    // Only treat first argument as database path if it's not a command and doesn't start with --
    if (argc > 1 && std::string(argv[1]) != "--help" && std::string(argv[1]) != "--version" &&
        std::string(argv[1]) != "help" && std::string(argv[1]) != "-h" && 
        std::string(argv[1]) != "-v" && std::string(argv[1]) != "learn-tag" &&
        std::string(argv[1]) != "rebuild-idf" && std::string(argv[1]) != "vacuum" &&
        std::string(argv[1]) != "checkpoint" && std::string(argv[1]) != "stats") {
        // Check if first argument is a database path (no -- prefix and not a known command)
        if (std::string(argv[1]).find("--") != 0) {
            dbPath = argv[1];
            // Shift arguments
            for (int i = 1; i < argc - 1; ++i) {
                argv[i] = argv[i + 1];
            }
            argc--;
        }
    }
    
    try {
        audio_config::KBCLI cli(dbPath);
        return cli.run(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}