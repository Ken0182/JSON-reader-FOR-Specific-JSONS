/**
 * @file main.cpp
 * @brief Multi-Dimensional Pointing System for Audio Configuration Assembly
 * @author AI Assistant
 * @version 1.1
 * 
 * A comprehensive system that implements 4-dimensional pointing for intelligent
 * audio configuration assembly with real-world compatibility validation.
 * 
 * STARTUP BUG FIX v1.1:
 * - Auto-detects execution location (root vs build/ directory)
 * - Resolves resource paths relative to executable location
 * - Supports CLI overrides for custom resource paths
 */

#include "audio_config_system.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <fstream>
#include <filesystem>

using namespace audio_config;
namespace fs = std::filesystem;

/**
 * @brief Print welcome message and system information
 */
void printWelcome() {
    std::cout << "Multi-Dimensional Audio Configuration System\n";
    std::cout << "=================================================================\n";
}

/**
 * @brief Print usage information
 */
void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [OPTIONS]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --weights <path>    Path to weights.json (default: auto-detected)\n";
    std::cout << "  --config <path>     Path to clean_config.json (default: auto-detected)\n";
    std::cout << "  --help              Show this help message\n\n";
    std::cout << "Auto-detection:\n";
    std::cout << "  The program automatically detects resource paths based on the\n";
    std::cout << "  executable location. Works from both repository root and build/\n";
    std::cout << "  directory without manual path configuration.\n";
}

/**
 * @brief Detect resource paths based on executable location
 * @param executablePath Path to the executable
 * @return Pair of (weights path, config path)
 */
std::pair<std::string, std::string> detectResourcePaths(const fs::path& executablePath) {
    fs::path execDir = executablePath.parent_path();
    fs::path currentDir = fs::current_path();
    
    // Default paths relative to repository root
    std::string weightsPath = "config/weights.json";
    std::string configPath = "data/clean_config.json";
    
    // Strategy 1: Check current working directory
    if (fs::exists(currentDir / "config" / "weights.json") && 
        fs::exists(currentDir / "data" / "clean_config.json")) {
        // Running from repository root
        weightsPath = (currentDir / "config" / "weights.json").string();
        configPath = (currentDir / "data" / "clean_config.json").string();
    }
    // Strategy 2: Check if current directory is build/
    else if (currentDir.filename() == "build") {
        // Running from build/ directory, go up one level
        fs::path rootDir = currentDir.parent_path();
        weightsPath = (rootDir / "config" / "weights.json").string();
        configPath = (rootDir / "data" / "clean_config.json").string();
    }
    // Strategy 3: Check relative to executable location
    else if (execDir.filename() == "build") {
        // Executable is in build/, use parent directory
        fs::path rootDir = execDir.parent_path();
        weightsPath = (rootDir / "config" / "weights.json").string();
        configPath = (rootDir / "data" / "clean_config.json").string();
    }
    // Strategy 4: Try from executable directory
    else if (fs::exists(execDir / "config" / "weights.json") &&
             fs::exists(execDir / "data" / "clean_config.json")) {
        weightsPath = (execDir / "config" / "weights.json").string();
        configPath = (execDir / "data" / "clean_config.json").string();
    }
    
    return {weightsPath, configPath};
}

/**
 * @brief Main application entry point with path auto-detection
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit code (0 for success)
 */
int main(int argc, char* argv[]) {
    try {
        // Parse command-line arguments
        std::string weightsPath;
        std::string configPath;
        
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            
            if (arg == "--help" || arg == "-h") {
                printUsage(argv[0]);
                return 0;
            } else if (arg == "--weights" && i + 1 < argc) {
                weightsPath = argv[++i];
            } else if (arg == "--config" && i + 1 < argc) {
                configPath = argv[++i];
            } else {
                std::cerr << "Unknown option: " << arg << "\n";
                printUsage(argv[0]);
                return 1;
            }
        }
        
        // Auto-detect paths if not provided via CLI
        if (weightsPath.empty() || configPath.empty()) {
            auto [detectedWeights, detectedConfig] = detectResourcePaths(argv[0]);
            if (weightsPath.empty()) weightsPath = detectedWeights;
            if (configPath.empty()) configPath = detectedConfig;
        }
        
        printWelcome();
        
        // Verify resource files exist
        if (!fs::exists(weightsPath)) {
            std::cerr << "Error: Weights file not found: " << weightsPath << "\n";
            std::cerr << "Use --weights <path> to specify custom location\n";
            return 1;
        }
        
        if (!fs::exists(configPath)) {
            std::cerr << "Error: Configuration database not found: " << configPath << "\n";
            std::cerr << "Use --config <path> to specify custom location\n";
            return 1;
        }
        
        // Initialize the main audio configuration system
        auto system = std::make_unique<AudioConfigSystem>(weightsPath);
        
        // v1.3: Detect SKD embedding index path (optional)
        fs::path execPath(argv[0]);
        fs::path execDir = execPath.parent_path();
        fs::path currentDir = fs::current_path();
        std::string skdPath;
        
        // v1.6: Try multiple locations for semantic database
        if (fs::exists(currentDir / "data" / "semantic.db")) {
            skdPath = (currentDir / "data" / "semantic.db").string();
        } else if (currentDir.filename() == "build") {
            fs::path rootDir = currentDir.parent_path();
            if (fs::exists(rootDir / "data" / "semantic.db")) {
                skdPath = (rootDir / "data" / "semantic.db").string();
            }
        } else if (execDir.filename() == "build") {
            fs::path rootDir = execDir.parent_path();
            if (fs::exists(rootDir / "data" / "semantic.db")) {
                skdPath = (rootDir / "data" / "semantic.db").string();
            }
        }
        
        // Fallback to :memory: if no DB found
        if (skdPath.empty()) {
            skdPath = ":memory:";
        }
        
        // Load configuration database (with optional SKD index)
        if (!system->initialize(configPath, skdPath)) {
            std::cerr << "Failed to initialize audio configuration system" << std::endl;
            return 1;
        }
        
        // Run interactive CLI
        system->runInteractiveCLI();
        
        std::cout << "\nThank you for using the Multi-Dimensional Audio Configuration System!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return 2;
    }
    
    return 0;
}