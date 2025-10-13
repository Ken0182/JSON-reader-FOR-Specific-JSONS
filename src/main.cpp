/**
 * @file main.cpp
 * @brief Multi-Dimensional Pointing System for Audio Configuration Assembly
 * @author AI Assistant
 * @version 1.0
 * 
 * A comprehensive system that implements 4-dimensional pointing for intelligent
 * audio configuration assembly with real-world compatibility validation.
 */

#include "audio_config_system.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>

using namespace audio_config;

/**
 * @brief Print welcome message and system information
 */
void printWelcome() {
    std::cout << R"(
============================================================
 Multi-Dimensional Audio Configuration System - Console View
============================================================

4D Pointing Dimensions:
  - Semantic: Embedding/tag similarity (FastText 100D)
  - Technical: Sample rate, plugin format, envelope compatibility
  - Musical Role: Lead, bass, pad, FX classification
  - Layering: Frequency, stereo, prominence analysis

Key Features:
  - Real-world DAW compatibility validation
  - AI-driven configuration assembly
  - Explainable recommendations
  - Interactive CLI with learning

Type 'help' for commands or 'examples' for usage patterns.
)" << std::endl;
}

/**
 * @brief Main application entry point
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit code (0 for success)
 */
int main(int /* argc */, char* /* argv */[]) {
    try {
        printWelcome();
        
        // Initialize the main audio configuration system
        auto system = std::make_unique<AudioConfigSystem>("config/weights.json");
        
        // Load configuration database
        if (!system->initialize("data/clean_config.json")) {
            std::cerr << "❌ Failed to initialize audio configuration system" << std::endl;
            return 1;
        }
        
        // Run interactive CLI
        system->runInteractiveCLI();
        
        std::cout << "\nThank you for using the Multi-Dimensional Audio Configuration System." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Unknown fatal error occurred" << std::endl;
        return 2;
    }
    
    return 0;
}