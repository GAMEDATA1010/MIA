// main.cpp
#include <iostream>
#include <string> // For std::string and std::getline
#include <cstdlib> // For getenv
#include "agent.h" // Only need agent_manager now

void enterConversation(bool *conversing);
void enterDevMode();

int main() {
    // --- IMPORTANT: Set your Google Gemini API Key as an environment variable ---
    // On Linux/macOS: export GEMINI_API_KEY="YOUR_API_KEY_HERE"
    // On Windows (Command Prompt): set GEMINI_API_KEY="YOUR_API_KEY_HERE"
    // On Windows (PowerShell): $env:GEMINI_API_KEY="YOUR_API_KEY_HERE"
    // Replace "YOUR_API_KEY_HERE" with your actual API key.
    // This should be done in your terminal *before* running the executable.

    bool devMode = false;
    bool conversing = true;

    // 1. Get API Key from environment variable
    const char* apiKeyCStr = std::getenv("GEMINI_API_KEY");
    if (apiKeyCStr == nullptr || std::string(apiKeyCStr).empty()) {
        std::cerr << "Error: GEMINI_API_KEY environment variable not set or is empty." << std::endl;
        std::cerr << "Please set it (e.g., 'export GEMINI_API_KEY=\"YOUR_API_KEY\"' on Linux/macOS)" << std::endl;
        std::cerr << "       (e.g., 'set GEMINI_API_KEY=\"YOUR_API_KEY\"' on Windows Command Prompt)" << std::endl;
        std::cerr << "       (e.g., '$env:GEMINI_API_KEY=\"YOUR_API_KEY\"' on Windows PowerShell)" << std::endl;
        return 1;
    }
    // No need to store geminiApiKey in main, AgentManager gets it directly.

    // 2. Define paths for configuration files
    const std::string baseConfigFilePath = "base_config.json"; // Main configuration file
    const std::string agentsFolderPath = "agents";             // Folder containing individual agent JSONs

    // 3. Initialize the AgentManager singleton
    // It will load base config, API key, initialize cURL, and load all agents.


    return 0;
}

void enterDevMode() {
	enum Options {AgentManager = 1, ChangeDefaults, EnableDebugging, Quit};
	Options choice = ChangeDefaults;
	int choice_int = 0;
	std::cout << "\n--- Welcome to the Developer Mode  ---" << std::endl;

	while(choice != Quit) {
		std::cout << "1) Agent Manager\n2) Change Default Configurations\n3) Enable Debugging\n4) Quit\n";
		std::cout << "Choice: ";
		
		std::cin >> choice_int;
		choice = static_cast<Options>(choice_int);
		if (choice != Quit) {
			switch (choice) {
				case AgentManager:
					std::cout << "Opening Agent Manager..." << std::endl;
					break;
				case ChangeDefaults:
					std::cout << "Opening Configuration..." << std::endl;
					break;
				case EnableDebugging:
					std::cout << "Debugging Enabled!" << std::endl;
					break;
				default:
					std::cout << "Invalid Option" << std::endl;
			}
		}
	}
	std::cout << "Exiting Developer Mode...";
}

void enterConversation(bool *conversing) {
    // 4. Start the interactive conversation loop
    std::cout << "\n--- Welcome to the General Assistant ---" << std::endl;
    std::cout << "Type your message and press Enter. Type 'quit' or 'exit' to end the conversation." << std::endl;

    std::string userPrompt;
    std::string agentId = "general_assistant"; // Assuming this is the only agent you're using for now

    while (true) {
        std::cout << "\nYou: ";
        // Use std::getline to read the entire line, including spaces
        std::getline(std::cin, userPrompt);

        if (userPrompt == "quit" || userPrompt == "exit") {
            std::cout << "Ending conversation. Goodbye!" << std::endl;
	    *conversing = false;
            break;
        }

        if (userPrompt == "$DevMode$") {
                std::cout << "Entering Developer Mode..." << std::endl;
                enterDevMode();
                // After returning from enterDevMode, clear any leftover newline
		// to prevent the next getline from reading an empty string.
            	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		break;
        }

        // Send the user's message to the General Assistant via the AgentManager

    }
}
