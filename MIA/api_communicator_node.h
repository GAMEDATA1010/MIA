#ifndef API_COMMUNICATOR_NODE_H
#define API_COMMUNICATOR_NODE_H

#include "node.h" // Inherits from Node
#include "api_communicator.h" // To interact with the singleton
#include <nlohmann/json.hpp>
#include <string>

/**
 * @class ApiCommunicatorNode
 * @brief A wrapper Node class for the ApiCommunicator singleton.
 *
 * This class allows the globally managed ApiCommunicator instance to
 * participate in the Linker's Node system, which uses std::unique_ptr
 * for memory management of its registered nodes. It delegates its Node
 * interface methods (push, pull, getId) to the ApiCommunicator singleton.
 */
class ApiCommunicatorNode : public Node {
public:
    /**
     * @brief Constructor for ApiCommunicatorNode.
     * @param id The unique identifier for this node (e.g., "api_communicator").
     */
    ApiCommunicatorNode(const std::string& id) : m_id(id) {}

    /**
     * @brief Returns the unique identifier for this node.
     * @return A const reference to the node's ID string.
     */
    const std::string& getId() const override { return m_id; }

    /**
     * @brief Pushes data to the underlying ApiCommunicator singleton.
     *
     * This method is called when data is sent to this node via the Linker.
     * It delegates the push operation to ApiCommunicator::getInstance().push().
     * @param data The JSON data to be processed by the ApiCommunicator.
     * @return True if the push operation was successful, false otherwise.
     */
    bool push(nlohmann::json data) override {
        return ApiCommunicator::getInstance().push(data);
    }

    /**
     * @brief Pulls data from the underlying ApiCommunicator singleton.
     *
     * This method is called to retrieve processed data from this node.
     * It delegates the pull operation to ApiCommunicator::getInstance().pull().
     * @return A JSON object containing the processed output from the ApiCommunicator.
     */
    nlohmann::json pull() override {
        return ApiCommunicator::getInstance().pull();
    }

private:
    std::string m_id; // Unique identifier for this node
};

#endif // API_COMMUNICATOR_NODE_H

