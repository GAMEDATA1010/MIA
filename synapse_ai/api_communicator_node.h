#ifndef API_COMMUNICATOR_NODE_H
#define API_COMMUNICATOR_NODE_H

#include "node.h"
#include "api_communicator.h" // To interact with the singleton
#include <nlohmann/json.hpp>
#include <string>

// ApiCommunicatorNode acts as a wrapper to allow the ApiCommunicator singleton
// to be registered and used within the Linker's Node system, which now
// manages Nodes using std::unique_ptr for ownership.
class ApiCommunicatorNode : public Node {
public:
    // Constructor initializes the node with its ID.
    ApiCommunicatorNode(const std::string& id) : m_id(id) {}

    // Returns the unique identifier for this node.
    const std::string& getId() const override { return m_id; }

    // Pushes data to the underlying ApiCommunicator singleton.
    // This method is called when data is sent to this node via the Linker.
    bool push(nlohmann::json data) override {
        // Delegates the push operation to the ApiCommunicator singleton.
        // Assumes ApiCommunicator's push method is designed to handle incoming data
        // (e.g., to initiate an LLM request).
        return ApiCommunicator::getInstance().push(data);
    }

    // Pulls data from the underlying ApiCommunicator singleton.
    // This method is called to retrieve processed data from this node.
    nlohmann::json pull() override {
        // Delegates the pull operation to the ApiCommunicator singleton.
        // Assumes ApiCommunicator's pull method retrieves the result of its processing
        // (e.g., the LLM's generated response).
        return ApiCommunicator::getInstance().pull();
    }

private:
    std::string m_id; // Unique identifier for this node
};
#endif // API_COMMUNICATOR_NODE_H

