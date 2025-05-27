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
        std::cerr << "Please set it (e.g., 'export GEMINI_API_KEY=\"YOUR_API_KEY\"' on Linux/macOS)" << std::endl;
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
        return 1;
    }

    // 4. Use agents by their ID to generate content
    std::cout << "\n--- Using General Assistant (ID: general_assistant) ---" << std::endl;
    AgentResponse assistantResponse = AgentManager::getInstance().generateContent("general_assistant", "What is the capital of Australia?");
    if (assistantResponse.success) {
        std::cout << "General Assistant says: " << assistantResponse.generatedText << std::endl;
    } else {
        std::cerr << "General Assistant Error: " << assistantResponse.errorMessage << " (HTTP " << assistantResponse.httpStatusCode << ")" << std::endl;
    }

    std::cout << "\n--- Using C++ Code Reviewer (ID: code_reviewer) ---" << std::endl;
    std::string codeSnippet = R"(
#include <iostream>
#include <vector>

void processData(int* arr, int size) {
    // This function takes a raw pointer and size.
    // It's easy to make off-by-one errors or forget to free memory.
    // Also, 'using namespace std;' is often avoided in headers.
    for (int i = 0; i <= size; ++i) { // Potential off-by-one error
        std::cout << arr[i] << std::endl;
    }
}

int main() {
    int* myArr = new int[10]; // Dynamically allocated array
    // ... use myArr ...
    // Missing 'delete[] myArr;' - memory leak!
    processData(myArr, 10);
    return 0;
}
)";
    AgentResponse reviewerResponse = AgentManager::getInstance().generateContent("code_reviewer", "Review this C++ code for potential issues and suggest improvements:\n\n" + codeSnippet);
    if (reviewerResponse.success) {
        std::cout << "C++ Code Reviewer says: \n" << reviewerResponse.generatedText << std::endl;
    } else {
        std::cerr << "C++ Code Reviewer Error: " << reviewerResponse.errorMessage << " (HTTP " << reviewerResponse.httpStatusCode << ")" << std::endl;
    }

    std::cout << "\n--- Attempting to use a non-existent agent ---" << std::endl;
    AgentResponse nonExistentResponse = AgentManager::getInstance().generateContent("non_existent_agent", "Hello?");
    if (!nonExistentResponse.success) {
        std::cerr << "Expected Error: " << nonExistentResponse.errorMessage << std::endl;
    }

    return 0;
}

