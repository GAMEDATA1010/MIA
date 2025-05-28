// main.cpp
#include <iostream>
#include <cstdlib> // For getenv
#include "agent_manager.h" // Only need agent_manager now

int main() {
    // --- IMPORTANT: Set your Google Gemini API Key as an environment variable ---
    // On Linux/macOS: export GEMINI_API_KEY="YOUR_API_KEY_HERE"
    // On Windows (Command Prompt): set GEMINI_API_KEY="YOUR_API_KEY_HERE"
    // On Windows (PowerShell): $env:GEMINI_API_KEY="YOUR_API_KEY_HERE"
    // Replace "YOUR_API_KEY_HERE" with your actual API key.
    // This should be done in your terminal *before* running the executable.

    // 1. Get API Key from environment variable
    const char* apiKeyCStr = std::getenv("GEMINI_API_KEY");
    if (apiKeyCStr == nullptr || std::string(apiKeyCStr).empty()) {
        std::cerr << "Error: GEMINI_API_KEY environment variable not set or is empty." << std::endl;
        std::cerr << "Please set it with this command\nexport GEMINI_API_KEY=\"YOUR_API_KEY\" on Linux/macOS" << std::endl;
        std::cerr << "GEMINI_API_KEY=\"YOUR_API_KEY\" on Windows (Command Prompt)" << std::endl;
        std::cerr << "$env:GEMINI_API_KEY=\"YOUR_API_KEY\" on Windows (PowerShell)" << std::endl;
        return 1;
    }
    std::string geminiApiKey = apiKeyCStr;

    // 2. Define paths for configuration files
    const std::string baseConfigFilePath = "base_config.json"; // Main configuration file
    const std::string agentsFolderPath = "agents";             // Folder containing individual agent JSONs

    // 3. Initialize the AgentManager singleton
    // It will load base config, API key, initialize cURL, and load all agents.
    if (!AgentManager::getInstance().initialize(baseConfigFilePath, agentsFolderPath)) {
        std::cerr << "Application Error: Failed to initialize AgentManager. Exiting." << std::endl;
        return 2;
    }

    // 4. Use agents by their ID to generate content
    std::string input;
    std::cout << "\n--- Using General Assistant (ID: general_assistant) ---" << std::endl;
    std::cout << "Prompt: ";
    std::cin >> input;

    AgentResponse assistantResponse = AgentManager::getInstance().generateContent("general_assistant", input);
    if (assistantResponse.success) {
        std::cout << "General Assistant says: " << assistantResponse.generatedText << std::endl;
    } else {
        std::cerr << "General Assistant Error: " << assistantResponse.errorMessage << " (HTTP " << assistantResponse.httpStatusCode << ")" << std::endl;
    }

    return 0;
}

