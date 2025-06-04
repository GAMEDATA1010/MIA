#ifndef NODE_H
#define NODE_H

#include <string>
#include <nlohmann/json.hpp> // For nlohmann::json type

/**
 * @class Node
 * @brief Abstract base class for all processing nodes in the Linker system.
 *
 * Defines the common interface for nodes that can receive data (push)
 * and provide processed data (pull).
 */
class Node {
public:
    /**
     * @brief Virtual destructor to ensure proper cleanup of derived classes.
     */
    virtual ~Node() = default;

    /**
     * @brief Pure virtual function to get the unique identifier of the node.
     * @return A const reference to the node's ID string.
     */
    virtual const std::string& getId() const = 0;

    /**
     * @brief Pure virtual function to push data into the node for processing.
     * @param data The JSON data to be processed by the node.
     * @return True if the data was successfully pushed and processed, false otherwise.
     */
    virtual bool push(nlohmann::json data) = 0;

    /**
     * @brief Pure virtual function to pull processed data from the node.
     * @return A JSON object containing the processed output from the node.
     */
    virtual nlohmann::json pull() = 0;
};

#endif // NODE_H

