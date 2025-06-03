#include "linker.h"
#include "api_communicator.h"
#include "agent.h" // Include Agent header to create Agent objects
#include "api_communicator_node.h" // Include the new wrapper node header
#include <iostream> // For logging and error messages
#include <fstream>  // For file input
#include <filesystem> // For directory traversal (C++17)

// Define the directory where agent JSON configurations are stored
const std::string AGENT_CONFIG_DIR = "agents";

// Static method to get the single instance of Linker (Singleton implementation)
Linker& Linker::getInstance() {
    static Linker instance; // Guaranteed to be initialized once and destroyed correctly
    return instance;
}

// Private constructor implementation
Linker::Linker() {
    // Any Linker-specific initialization can go here
}

bool Linker::initialize() {
    // 1. Get API Key from environment variable
    const char* apiKeyCStr = std::getenv("GEMINI_API_KEY");
    if (apiKeyCStr == nullptr || std::string(apiKeyCStr).empty()) {
        std::cerr << "Error: GEMINI_API_KEY environment variable not set or is empty." << std::endl;
        std::cerr << "Please set it (e.g., 'export GEMINI_API_KEY=\\\"YOUR_API_KEY\\\"' on Linux/macOS)" << std::endl;
        std::cerr << "       (e.g., 'set GEMINI_API_KEY=\\\"YOUR_API_KEY\\\"' on Windows Command Prompt)" << std::endl;
        std::cerr << "       (e.g., '$env:GEMINI_API_KEY=\\\"YOUR_API_KEY\\\"' on Windows PowerShell)" << std::endl;
        return false; // Indicate an error and exit
    }

    // 2. Initialize ApiCommunicator (Singleton instance)
    // ApiCommunicator is a Singleton and its lifetime is managed internally.
    // Agents will interact with it directly via ApiCommunicator::getInstance().
    ApiCommunicator& apiCommunicator = ApiCommunicator::getInstance();
    if (!apiCommunicator.initialize()) {
        std::cerr << "Failed to initialize ApiCommunicator. Exiting." << std::endl;
        return false; // Indicate an error and exit
    }
    std::cout << "Linker: ApiCommunicator initialized." << std::endl;

    // Register the ApiCommunicator through its wrapper Node.
    // This allows ApiCommunicator to be part of the Linker's Node map,
    // while the Linker correctly manages the lifetime of the wrapper.
    Linker::getInstance().registerNode("api_communicator", std::make_unique<ApiCommunicatorNode>("api_communicator"));
    std::cout << "Linker: ApiCommunicatorNode registered with ID 'api_communicator'." << std::endl;


    // 3. Load Agents from JSON configuration files
    std::cout << "Linker: Loading agents from directory: " << AGENT_CONFIG_DIR << std::endl;
    namespace fs = std::filesystem;
    try {
        if (!fs::exists(AGENT_CONFIG_DIR) || !fs::is_directory(AGENT_CONFIG_DIR)) {
            std::cerr << "Linker Error: Agent configuration directory '" << AGENT_CONFIG_DIR << "' not found or is not a directory." << std::endl;
            return false;
        }

        for (const auto& entry : fs::directory_iterator(AGENT_CONFIG_DIR)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string filePath = entry.path().string();
                std::cout << "Linker: Found agent config file: " << filePath << std::endl;

                std::ifstream file(filePath);
                if (!file.is_open()) {
                    std::cerr << "Linker Error: Could not open file " << filePath << std::endl;
                    continue; // Skip to the next file
                }

                nlohmann::json agentConfig;
                try {
                    file >> agentConfig; // Parse the JSON file
                } catch (const nlohmann::json::parse_error& e) {
                    std::cerr << "Linker Error: JSON parse error in " << filePath << ": " << e.what() << std::endl;
                    continue;
                }

                // Extract agent properties
                std::string id;
                std::string name;
                LLMParameters params;

                try {
                    id = agentConfig.at("id").get<std::string>();
                    name = agentConfig.at("name").get<std::string>();

                    const auto& param_json = agentConfig.at("parameters");
                    params.model = param_json.at("model").get<std::string>();
                    params.temperature = param_json.at("temperature").get<float>();
                    params.topP = param_json.at("top_p").get<float>(); // Note: top_p in JSON, topP in struct
                    params.topK = param_json.at("top_k").get<int>();   // Note: top_k in JSON, topK in struct
                    params.maxOutputTokens = param_json.at("max_output_tokens").get<int>();
                    params.instructions = param_json.at("instructions").get<std::string>();
                    // maxHistoryTurns is not in your general_assistant.json, so provide a default or handle its absence
                    params.maxHistoryTurns = param_json.value("max_history_turns", 5); // Default to 5 if not present

                } catch (const nlohmann::json::exception& e) {
                    std::cerr << "Linker Error: Missing or invalid field in agent config " << filePath << ": " << e.what() << std::endl;
                    continue; // Skip this file
                }

                // Create and register the Agent
                // std::make_unique creates a unique_ptr and constructs the Agent within it
                auto agent = std::make_unique<Agent>(id, name, params);
                Linker::getInstance().registerNode(id, std::move(agent)); // Transfer ownership

            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Linker Error: Filesystem error while loading agents: " << e.what() << std::endl;
        return false;
    }

    return true; // Indicate success
}

// Registers a Node with its ID. Takes ownership of the unique_ptr.
void Linker::registerNode(const std::string& nodeId, std::unique_ptr<Node> nodePtr) {
    if (nodePtr == nullptr) {
        std::cerr << "Linker Error: Attempted to register a nullptr for Node ID: " << nodeId << std::endl;
        return;
    }
    if (m_registeredNodes.count(nodeId)) {
        std::cerr << "Linker Warning: Node ID '" << nodeId << "' already registered. Overwriting." << std::endl;
    }
    m_registeredNodes[nodeId] = std::move(nodePtr); // Transfer ownership
    std::cout << "Linker: Node '" << nodeId << "' registered." << std::endl;
}

// Sends data to a single target Node
bool Linker::sendData(const std::string& toId, nlohmann::json data) {
    auto it = m_registeredNodes.find(toId);
    if (it == m_registeredNodes.end()) {
        std::cerr << "Linker Error: Destination Node ID '" << toId << "' not found." << std::endl;
        return false;
    }

    // Access the raw pointer from the unique_ptr to call push()
    Node* targetNode = it->second.get();
    std::cout << "Linker: Sending data to Node '" << toId << "'" << std::endl;
    return targetNode->push(data);
}

bool Linker::send(const std::string& toId, const std::string& fromId) {
    nlohmann::json data = Linker::getInstance().fetch(fromId);

    return Linker::getInstance().sendData(toId, data);
}

// Sends data through a sequence of Nodes, where output of one becomes input for the next
bool Linker::sendDataStream(const std::vector<std::string>& nodeIds, nlohmann::json initialData) {
    if (nodeIds.empty()) {
        std::cerr << "Linker Error: sendStream called with empty node ID list." << std::endl;
        return false;
    }

    nlohmann::json currentData = initialData;
    bool success = true;

    for (size_t i = 0; i < nodeIds.size(); ++i) {
        const std::string& nodeId = nodeIds[i];
        auto it = m_registeredNodes.find(nodeId);
        if (it == m_registeredNodes.end()) {
            std::cerr << "Linker Error: Node ID '" << nodeId << "' in stream not found." << std::endl;
            success = false;
            break;
        }

        // Access the raw pointer from the unique_ptr
        Node* currentNode = it->second.get();
        std::cout << "Linker: Processing stream - sending data to Node '" << nodeId << "'" << std::endl;

        // Push data to the current node for processing
        if (!currentNode->push(currentData)) {
            std::cerr << "Linker Error: Node '" << nodeId << "' failed to process input data during stream." << std::endl;
            success = false;
            break;
        }

        // Pull processed data from the current node to be used as input for the next node in the stream
        currentData = currentNode->pull();
        std::cout << "Linker: Received processed data from Node '" << nodeId << "'" << std::endl;
    }
    return success;
}

bool Linker::sendStream(const std::vector<std::string>& nodeIds, const std::string& fromId) {
    nlohmann::json data = Linker::getInstance().fetch(fromId);

    return Linker::getInstance().sendDataStream(nodeIds, data);
}

// Sends the same data to multiple target Nodes
bool Linker::sendDataMulti(const std::vector<std::string>& toIds, nlohmann::json data) {
    if (toIds.empty()) {
        std::cerr << "Linker Warning: sendMulti called with empty destination list. No data sent." << std::endl;
        return true;
    }

    bool allSentSuccessfully = true;
    for (const std::string& toId : toIds) {
        // Reuse the single send method for each recipient
        if (!sendData(toId, data)) { // send() already handles unique_ptr dereferencing
            allSentSuccessfully = false; // If any single send fails, the overall multi-send fails
        }
    }
    return allSentSuccessfully;
    
}

bool Linker::sendMulti(const std::vector<std::string>& nodeIds, const std::string& fromId) {
    nlohmann::json data = Linker::getInstance().fetch(fromId);

    return Linker::getInstance().sendDataMulti(nodeIds, data);
}

nlohmann::json Linker::fetch(const std::string& nodeId) {
    auto it = m_registeredNodes.find(nodeId);
    if (it == m_registeredNodes.end()) {
        std::cerr << "Linker Error: Destination Node ID '" << nodeId << "' not found." << std::endl;
    }

    // Access the raw pointer from the unique_ptr to call push()
    Node* targetNode = it->second.get();
    std::cout << "Linker: Fetching data from Node '" << nodeId << "'" << std::endl;
    return targetNode->pull();

}
