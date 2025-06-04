// main.cpp
#include <iostream>
#include <string> // For std::string and std::getline
#include <cstdlib> // For getenv
#include <limits> // For std::numeric_limits (used for std::cin.ignore)
#include <chrono> 
#include "agent.h"
#include "api_communicator.h"
#include "linker.h" // Include the Linker header
#include "timer.h"

// Forward declarations of functions used in main
void enterConversation(bool *conversing, Linker& linker);
void enterDevMode();
nlohmann::json agentGenerate(std::string agentId, std::string message, Linker& linker);

int main() {
    // --- IMPORTANT: Set your Google Gemini API Key as an environment variable ---
    // On Linux/macOS: export GEMINI_API_KEY="YOUR_API_KEY_HERE"
    // On Windows (Command Prompt): set GEMINI_API_KEY="YOUR_API_KEY_HERE"
    // On Windows (PowerShell): $env:GEMINI_API_KEY="YOUR_API_KEY_HERE"
    // Replace "YOUR_API_KEY_HERE" with your actual API key.
    // This should be done in your terminal *before* running the executable.

    bool devMode = false; // Set to true to start directly in dev mode if needed for testing
    bool conversing = true; // Controls the main conversation loop

    Timer timer;
    Linker& linker = Linker::getInstance();
    if (!linker.initialize()) {
        std::cerr << "Failed to initialize Linker. Exiting." << std::endl;
        return 1; // Indicate an error and exit
    }

    timer.capture("Linker Initialization");
    std::cout << "Linker Initialized!" << std::endl; 
    timer.log();

    // 3. Decide whether to enter conversation mode or developer mode
    // (You can change `devMode` flag above or implement command-line argument parsing)
    if (devMode) {
        enterDevMode();
    } else {
        enterConversation(&conversing, linker);
    }

    // Program exits after conversation or dev mode.
    return 0;
}

// Helper function to handle developer mode interactions
void enterDevMode() {
    // Define a simple menu for developer mode options
    enum DevModeOption {
        Exit = 0,
        TestAgentCommunication,
        TestLinkerStream,
        TestLinkerMulti,
        LoadConfiguration, // Placeholder
        EnableDebugging    // Placeholder for toggling debug
    };

    int option;
    std::string userPrompt;
    bool devModeActive = true;
    DevModeOption choice;
    
    switch (static_cast<DevModeOption>(option)) {
            case Exit:
                devModeActive = false;
                break;
            case TestAgentCommunication: {
                std::cout << "Enter prompt for general_assistant agent: ";
                std::getline(std::cin, userPrompt);
                nlohmann::json message = {{"type", "user_input"}, {"content", userPrompt}};
                // Use Linker to send the message to the "general_assistant" agent
                bool success = Linker::getInstance().send("general_assistant", message);
                std::cout << "Send to general_assistant via Linker: " << (success ? "SUCCESS" : "FAILED") << std::endl;

                // For testing, we directly pull the response from the agent after it has processed
                // In a production system, the agent would use Linker to send its response to another Node (e.g., a display node).
                auto it = Linker::getInstance().m_registeredNodes.find("general_assistant");
                if (it != Linker::getInstance().m_registeredNodes.end()) {
                    // Access the raw pointer from the unique_ptr
                    Node* agentNode = it->second.get(); // Corrected: use .get()
                    nlohmann::json agentResponse = agentNode->pull(); // Get the processed output
                    std::cout << "Agent Response (via direct pull for test): " << agentResponse.dump(2) << std::endl;
                }
                break;
            }
            case TestLinkerStream: {
                std::cout << "Testing Linker Stream (Conceptual: Initial Data -> Agent -> ApiCommunicator):" << std::endl;
                std::cout << "Enter initial data for the stream: ";
                std::getline(std::cin, userPrompt);
                nlohmann::json initialStreamData = {{"stream_start", true}, {"data", userPrompt}};

                // Example stream path: User input -> General Agent -> ApiCommunicatorNode
                std::vector<std::string> streamPath = {"general_assistant", "api_communicator"};

                bool success = Linker::getInstance().sendStream(streamPath, initialStreamData);
                std::cout << "Stream test result: " << (success ? "SUCCESS" : "FAILED") << std::endl;

                // After the stream, the final output would be in the last node's m_data_out.
                if (success) {
                    auto it_last = Linker::getInstance().m_registeredNodes.find("api_communicator"); // Check last node in stream
                    if (it_last != Linker::getInstance().m_registeredNodes.end()) {
                        Node* lastNode = it_last->second.get(); // Corrected: use .get()
                        std::cout << "Final Stream Output (from api_communicator): " << lastNode->pull().dump(2) << std::endl;
                    }
                }
                break;
            }
            case TestLinkerMulti: {
                std::cout << "Testing Linker Multi-send (Sending same data to Agent & API Communicator):" << std::endl;
                std::cout << "Enter data for multi-send: ";
                std::getline(std::cin, userPrompt);
                nlohmann::json multiData = {{"action", "broadcast"}, {"content", userPrompt}};

                std::vector<std::string> recipients = {"general_assistant", "api_communicator"};
                bool success = Linker::getInstance().sendMulti(recipients, multiData);
                std::cout << "Multi-send test result: " << (success ? "SUCCESS" : "FAILED") << std::endl;
                // In a real scenario, you would then check outputs from each recipient separately.
                break;
            }
            case LoadConfiguration:
                std::cout << "Loading configuration... (Not Implemented Yet)" << std::endl;
                break;
            case EnableDebugging:
                // This would ideally call a setter in ApiCommunicator
                std::cout << "Debugging configuration... (ApiCommunicator) Current: " << (ApiCommunicator::getInstance().getDebuggingMode() ? "Enabled" : "Disabled") << std::endl;
                // You would need to add a public setter like ApiCommunicator::getInstance().setDebuggingMode(true/false);
                break;
            default:
                std::cout << "Invalid Option" << std::endl;
    }
    std::cout << "Exiting Developer Mode...";
}

// Function to handle the main conversation loop
void enterConversation(bool *conversing, Linker& linker) {
    std::cout << "\n--- Welcome to the General Assistant ---\n" << std::endl;
    std::cout << "Type your message and press Enter. Type 'quit' or 'exit' to end the conversation." << std::endl;

    std::string userPrompt;
    std::string agent_mia = "new_assistant";
    std::string agent_optimizer = "general_assistant"; // The ID of the primary agent for conversation

    auto start = std::chrono::high_resolution_clock::now();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    while (true) {
        std::cout << "\nYou: ";
        std::getline(std::cin, userPrompt);

        if (userPrompt == "quit" || userPrompt == "exit") {
            std::cout << "Ending conversation. Goodbye!" << std::endl;
            *conversing = false;
            break;
        }

        if (userPrompt == "$DevMode$") {
            std::cout << "Entering Developer Mode..." << std::endl;
            enterDevMode();
            // After returning from enterDevMode, clear any leftover newline to prevent
            // the next getline from reading an empty string unintentionally.
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\\n');
            break; // Exit conversation loop to allow main to decide next action
        }

        // --- Retrieve and display agent's response ---
        // In a complete Linker-based system, the agent's response would be sent back
        // via the Linker to a dedicated "console output" Node. For this example,
        // we directly pull from the agent's output after its push method has executed.
	
	start = std::chrono::high_resolution_clock::now();

	linker.sendData(agent_optimizer, {{"type","user_input"}, {"content", userPrompt}});
	linker.send(agent_mia, agent_optimizer);
	
	end = std::chrono::high_resolution_clock::now();

	duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

	std::cout << linker.fetch(agent_optimizer)["generated_text"].get<std::string>() << std::endl;

	std::cout << linker.fetch(agent_mia)["generated_text"].get<std::string>() << std::endl;
    
        std::cout << "Execution time was: " << duration.count() << " milliseconds." << std::endl;
    }
}
