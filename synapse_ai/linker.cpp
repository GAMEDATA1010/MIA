#include "linker.h"
#include <iostream> // For logging and error messages

// Static method to get the single instance of Linker (Singleton implementation)
Linker& Linker::getInstance() {
    static Linker instance; // Guaranteed to be initialized once and destroyed correctly
    return instance;
}

// Private constructor implementation
Linker::Linker() {
    // Any Linker-specific initialization can go here
}

// Registers a Node with its ID
void Linker::registerNode(const std::string& nodeId, Node* nodePtr) {
    if (nodePtr == nullptr) {
        std::cerr << "Linker Error: Attempted to register a nullptr for Node ID: " << nodeId << std::endl;
        return;
    }
    if (m_registeredNodes.count(nodeId)) {
        std::cerr << "Linker Warning: Node ID '" << nodeId << "' already registered. Overwriting." << std::endl;
    }
    m_registeredNodes[nodeId] = nodePtr;
    std::cout << "Linker: Node '" << nodeId << "' registered." << std::endl;
}

// Sends data to a single target Node
bool Linker::send(const std::string& toId, nlohmann::json data) {
    auto it = m_registeredNodes.find(toId);
    if (it == m_registeredNodes.end()) {
        std::cerr << "Linker Error: Destination Node ID '" << toId << "' not found." << std::endl;
        return false;
    }

    Node* targetNode = it->second;
    std::cout << "Linker: Sending data to Node '" << toId << "'" << std::endl;
    // Push data to the target Node's input
    return targetNode->push(data);
}

// Sends data through a sequence of Nodes, where output of one becomes input for the next
bool Linker::sendStream(const std::vector<std::string>& nodeIdsInOrder, nlohmann::json initialData) {
    if (nodeIdsInOrder.empty()) {
        std::cerr << "Linker Error: sendStream called with empty node ID list." << std::endl;
        return false;
    }

    nlohmann::json currentData = initialData;
    bool success = true;

    for (size_t i = 0; i < nodeIdsInOrder.size(); ++i) {
        const std::string& nodeId = nodeIdsInOrder[i];
        auto it = m_registeredNodes.find(nodeId);
        if (it == m_registeredNodes.end()) {
            std::cerr << "Linker Error: Node ID '" << nodeId << "' in stream not found." << std::endl;
            success = false;
            break;
        }

        Node* currentNode = it->second;
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

// Sends the same data to multiple target Nodes
bool Linker::sendMulti(const std::vector<std::string>& toIds, nlohmann::json data) {
    if (toIds.empty()) {
        std::cerr << "Linker Warning: sendMulti called with empty destination list. No data sent." << std::endl;
        return true;
    }

    bool allSentSuccessfully = true;
    for (const std::string& toId : toIds) {
        // Reuse the single send method for each recipient
        if (!send(toId, data)) {
            allSentSuccessfully = false; // If any single send fails, the overall multi-send fails
        }
    }
    return allSentSuccessfully;
}
