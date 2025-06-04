#ifndef LINKER_H
#define LINKER_H

#include <nlohmann/json.hpp>
#include <string>
#include <map>
#include <vector>
#include <memory> // Required for std::unique_ptr
#include "node.h" // Include Node base class

/**
 * @class Linker
 * @brief Singleton class responsible for managing and orchestrating communication
 * between different processing Nodes in the system.
 *
 * It holds ownership of registered Node objects via unique_ptr and provides
 * methods to send data to single nodes, through streams of nodes, or to multiple nodes.
 */
class Linker {
public:
    // Singleton pattern: Get the single instance of the Linker
    static Linker& getInstance();

    // Delete copy constructor and assignment operator to prevent copying
    Linker(const Linker&) = delete;
    Linker& operator=(const Linker&) = delete;

    /**
     * @brief Initializes the Linker, including API communication and loading agents.
     * @return True if initialization is successful, false otherwise.
     */
    bool initialize();

    /**
     * @brief Registers a Node with the Linker, taking ownership of the unique_ptr.
     * @param nodeId The unique identifier string for the node.
     * @param nodePtr A std::unique_ptr to the Node object.
     */
    void registerNode(const std::string& nodeId, std::unique_ptr<Node> nodePtr);

    /**
     * @brief Sends data from a source Node's output to a single target Node.
     * Fetches data from 'fromId' and sends it to 'toId'.
     * @param toId The ID of the destination Node.
     * @param fromId The ID of the source Node from which to fetch data.
     * @return True if the data was successfully sent, false otherwise.
     */
    bool send(const std::string& toId, const std::string& fromId);

    /**
     * @brief Sends a direct JSON data payload to a single target Node.
     * @param toId The ID of the destination Node.
     * @param data The JSON data to send.
     * @return True if the data was successfully sent, false otherwise.
     */
    bool sendData(const std::string& toId, nlohmann::json data);

    /**
     * @brief Sends data through a sequence of Nodes.
     * The output of one node becomes the input for the next.
     * @param nodeIdsInOrder A vector of node IDs defining the processing order.
     * @param initialData The initial JSON data to start the stream.
     * @return True if the entire stream processing was successful, false otherwise.
     */
    bool sendDataStream(const std::vector<std::string>& nodeIdsInOrder, nlohmann::json initialData);

    /**
     * @brief Sends data through a sequence of Nodes, fetching initial data from a source node.
     * @param nodeIdsInOrder A vector of node IDs defining the processing order.
     * @param fromId The ID of the source Node from which to fetch initial data.
     * @return True if the entire stream processing was successful, false otherwise.
     */
    bool sendStream(const std::vector<std::string>& nodeIdsInOrder, const std::string& fromId);

    /**
     * @brief Sends the same data to multiple target Nodes simultaneously.
     * @param toIds A vector of node IDs to which the data should be sent.
     * @param data The JSON data to send to all target nodes.
     * @return True if all sends were successful, false otherwise.
     */
    bool sendDataMulti(const std::vector<std::string>& toIds, nlohmann::json data);

    /**
     * @brief Sends the same data to multiple target Nodes simultaneously, fetching data from a source node.
     * @param toIds A vector of node IDs to which the data should be sent.
     * @param fromId The ID of the source Node from which to fetch data.
     * @return True if all sends were successful, false otherwise.
     */
    bool sendMulti(const std::vector<std::string>& toIds, const std::string& fromId);

    /**
     * @brief Fetches the last output data from a specified Node.
     * @param nodeId The ID of the Node from which to fetch data.
     * @return The JSON output data from the specified Node.
     */
    nlohmann::json fetch(const std::string& nodeId);

    // Public for testing purposes (consider making private with a getter in production)
    // Stores registered Node objects, owning their lifetime via unique_ptr.
    std::map<std::string, std::unique_ptr<Node>> m_registeredNodes;

private:
    // Private constructor for Singleton pattern
    Linker();
    // Private destructor for Singleton (default is typically fine)
    ~Linker() = default;
};

#endif // LINKER_H

