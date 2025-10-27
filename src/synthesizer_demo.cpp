/**
 * @file synthesizer_demo.cpp
 * @brief Demo program for testing the sound synthesizer
 * @author AI Assistant
 * @version 1.0
 */

#include "sound_synthesizer.hpp"
#include "audio_config_system.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <signal.h>

using namespace audio_config;

// Global synthesizer manager for signal handling
SoundSynthesizerManager* g_synthesizerManager = nullptr;

void signalHandler(int /* signal */) {
    if (g_synthesizerManager) {
        std::cout << "\nStopping synthesizer..." << std::endl;
        g_synthesizerManager->stopPlayback();
    }
    exit(0);
}

void printUsage() {
    std::cout << "Sound Synthesizer Demo" << std::endl;
    std::cout << "Usage: synthesizer_demo <config_file>" << std::endl;
    std::cout << "Example: synthesizer_demo clean_config.json" << std::endl;
    std::cout << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  Press Enter to play a note" << std::endl;
    std::cout << "  Press 'q' + Enter to quit" << std::endl;
    std::cout << "  Press 's' + Enter to stop all notes" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printUsage();
        return 1;
    }
    
    std::string configFile = argv[1];
    
    // Set up signal handling
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        // Initialize audio config system
        std::cout << "Initializing Audio Config System..." << std::endl;
        AudioConfigSystem audioSystem("config/weights.json");
        
        if (!audioSystem.initialize(configFile)) {
            std::cerr << "Failed to initialize Audio Config System" << std::endl;
            return 1;
        }
        
        // Search for available configurations
        std::cout << "Searching for available configurations..." << std::endl;
        auto results = audioSystem.searchConfigurations("", 10);
        
        if (results.empty()) {
            std::cerr << "No configurations found" << std::endl;
            return 1;
        }
        
        // Display available configurations
        std::cout << "\nAvailable configurations:" << std::endl;
        for (size_t i = 0; i < results.size(); ++i) {
            std::cout << "  " << (i + 1) << ". " << results[i].first->getName() << std::endl;
        }
        
        // Select first configuration
        std::cout << "\nSelecting first configuration: " << results[0].first->getName() << std::endl;
        auto selectedConfig = results[0].first;
        
        if (!selectedConfig) {
            std::cerr << "No configuration selected" << std::endl;
            return 1;
        }
        
        // Initialize synthesizer
        std::cout << "Initializing Sound Synthesizer..." << std::endl;
        SoundSynthesizerManager synthesizerManager;
        g_synthesizerManager = &synthesizerManager;
        
        if (!synthesizerManager.initialize()) {
            std::cerr << "Failed to initialize synthesizer" << std::endl;
            return 1;
        }
        
        // Start playing the configuration
        std::cout << "Starting synthesizer..." << std::endl;
        if (!synthesizerManager.playConfiguration(*selectedConfig)) {
            std::cerr << "Failed to start synthesizer" << std::endl;
            return 1;
        }
        
        std::cout << "\nSynthesizer is running!" << std::endl;
        std::cout << "Press Enter to play a note, 'q' to quit, 's' to stop all notes" << std::endl;
        
        // Interactive loop
        std::string input;
        int noteNumber = 60; // Middle C
        
        while (true) {
            std::cout << "> ";
            std::getline(std::cin, input);
            
            if (input == "q" || input == "quit") {
                break;
            } else if (input == "s" || input == "stop") {
                synthesizerManager.getSynthesizer()->allNotesOff();
                std::cout << "All notes stopped" << std::endl;
            } else if (input.empty()) {
                // Play a note
                synthesizerManager.getSynthesizer()->noteOn(noteNumber, 0.8);
                std::cout << "Playing note " << noteNumber << " (frequency: " 
                         << (440.0 * std::pow(2.0, (noteNumber - 69) / 12.0)) << " Hz)" << std::endl;
                
                // Stop note after 1 second
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                synthesizerManager.getSynthesizer()->noteOff(noteNumber);
            } else if (input == "help") {
                printUsage();
            } else {
                std::cout << "Unknown command. Press Enter for help." << std::endl;
            }
        }
        
        std::cout << "Stopping synthesizer..." << std::endl;
        synthesizerManager.stopPlayback();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Demo completed." << std::endl;
    return 0;
}