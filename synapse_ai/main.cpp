// main.cpp
#include <iostream>
#include <string> // For std::string and std::getline
#include <cstdlib> // For getenv
#include <limits> // For std::numeric_limits (used for std::cin.ignore)
#include "agent.h"
#include "api_communicator.h"
#include "linker.h" // Include the Linker header

// Forward declarations of functions used in main
void enterConversation(bool *conversing);
void enterDevMode();

int main() {
    // --- IMPORTANT: Set your Google Gemini API Key as an environment variable ---
    // On Linux/macOS: export GEMINI_API_KEY="YOUR_API_KEY_HERE"
    // On Windows (Command Prompt): set GEMINI_API_KEY="YOUR_API_KEY_HERE"
    // On Windows (PowerShell): $env:GEMINI_API_KEY="YOUR_API_KEY_HERE"
    // Replace "YOUR_API_KEY_HERE" with your actual API key.
    // This should be done in your terminal *before* running the executable.

    bool devMode = false; // Set to true to start directly in dev mode if needed for testing
    bool conversing = true; // Controls the main conversation loop

    // 1. Get API Key from environment variable
    const char* apiKeyCStr = std::getenv("GEMINI_API_KEY");
    if (apiKeyCStr == nullptr || std::string(apiKeyCStr).empty()) {
        std::cerr << "Error: GEMINI_API_KEY environment variable not set or is empty." << std::endl;
        std::cerr << "Please set it (e.g., 'export GEMINI_API_KEY=\\\"YOUR_API_KEY\\\"' on Linux/macOS)" << std::endl;
        std::cerr << "       (e.g., 'set GEMINI_API_KEY=\\\"YOUR_API_KEY\\\"' on Windows Command Prompt)" << std::endl;
        std::cerr << "       (e.g., '$env:GEMINI_API_KEY=\\\"YOUR_API_KEY\\\"' on Windows PowerShell)" << std::endl;
        return 1; // Indicate an error and exit
    }

    // 2. Initialize ApiCommunicator (Singleton instance)
    ApiCommunicator& apiCommunicator = ApiCommunicator::getInstance();
    if (!apiCommunicator.initialize()) {
        std::cerr << "Failed to initialize ApiCommunicator. Exiting." << std::endl;
        return 1; // Indicate an error and exit
    }

    // --- Register Nodes with the Linker ---
    // For demonstration, we manually create an Agent and register it.
    // In a more complex system, this would be managed by a configuration loader or AgentManager.
    LLMParameters defaultParams = {
        "gemini-2.0-flash-lite", // Example model name
        0.7f,         // Temperature
        0.9f,         // Top P
        1,            // Top K
        1024,         // Max Output Tokens
        5,            // Max History Turns
	""	      // System Instructions
    };
    Agent generalAssistantAgent("general_assistant", "General Assistant", defaultParams);

    // Register instances with the Linker using their unique IDs
    Linker::getInstance().registerNode(generalAssistantAgent.getId(), &generalAssistantAgent);
    // The ApiCommunicator is also a Node; register it by a descriptive ID.
    Linker::getInstance().registerNode("api_communicator", &ApiCommunicator::getInstance());


    // 3. Decide whether to enter conversation mode or developer mode
    // (You can change `devMode` flag above or implement command-line argument parsing)
    if (devMode) {
        enterDevMode();
    } else {
        enterConversation(&conversing);
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

    while (devModeActive) {
        std::cout << "\n--- Developer Mode ---" << std::endl;
        std::cout << "1. Test Agent Communication (User Prompt -> General Agent -> Response)" << std::endl;
        std::cout << "2. Test Linker Stream (Initial Data -> Node1 -> Node2 -> ...)" << std::endl;
        std::cout << "3. Test Linker Multi-send (Data to multiple Nodes)" << std::endl;
        std::cout << "4. Load Configuration (Not Implemented)" << std::endl;
        std::cout << "5. Enable/Disable Debugging (ApiCommunicator)" << std::endl;
        std::cout << "0. Exit Developer Mode" << std::endl;
        std::cout << "Enter option: ";
        std::cin >> option;

        // Clear the newline character left in the buffer by std::cin >> option
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\\n');

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
                    Node* agentNode = it->second;
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

                // Example stream path: User input -> General Agent -> ApiCommunicator (to simulate LLM call)
                // In a real scenario, this stream would represent a complex workflow.
                std::vector<std::string> streamPath = {"general_assistant", "api_communicator"};

                bool success = Linker::getInstance().sendStream(streamPath, initialStreamData);
                std::cout << "Stream test result: " << (success ? "SUCCESS" : "FAILED") << std::endl;

                // After the stream, the final output would be in the last node's m_data_out.
                if (success) {
                    auto it_last = Linker::getInstance().m_registeredNodes.find("api_communicator");
                    if (it_last != Linker::getInstance().m_registeredNodes.end()) {
                        Node* lastNode = it_last->second;
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
    }
    std::cout << "Exiting Developer Mode...";
}

// Function to handle the main conversation loop
void enterConversation(bool *conversing) {
    std::cout << "\n--- Welcome to the General Assistant ---\n" << std::endl;
    std::cout << "Type your message and press Enter. Type 'quit' or 'exit' to end the conversation." << std::endl;

    std::string userPrompt;
    std::string agentId = "general_assistant"; // The ID of the primary agent for conversation

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

        // --- Use Linker to send user prompt to the general_assistant agent ---
        nlohmann::json message = {{"type", "user_input"}, {"content", userPrompt}};
        if (!Linker::getInstance().send(agentId, message)) {
            std::cerr << "Failed to send prompt to agent via Linker. Skipping response." << std::endl;
	    std::cout << ApiCommunicator::getInstance().pull() << std::endl;
	    continue; // Skip to next iteration if send failed
        }

        // --- Retrieve and display agent's response ---
        // In a complete Linker-based system, the agent's response would be sent back
        // via the Linker to a dedicated "console output" Node. For this example,
        // we directly pull from the agent's output after its push method has executed.
        auto it = Linker::getInstance().m_registeredNodes.find(agentId);
        if (it != Linker::getInstance().m_registeredNodes.end()) {
            Node* targetAgentNode = it->second;
            nlohmann::json agentResponse = targetAgentNode->pull(); // Get the response from the agent
            // Assuming the agent's response JSON contains a "generated_text" field
            if (agentResponse.contains("generated_text") && agentResponse["generated_text"].is_string()) {
                std::cout << "Mia: " << agentResponse["generated_text"].get<std::string>() << std::endl;
            } else {
                std::cout << "Mia: (No recognizable text response. Raw output: " << agentResponse.dump(2) << ")" << std::endl;
            }
        } else {
            std::cerr << "Error: General Assistant Agent not found for response retrieval." << std::endl;
        }
    }
}
