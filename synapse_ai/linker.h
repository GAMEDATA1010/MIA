#ifndef LINKER_H
#define LINKER_H

#include <nlohmann/json.hpp>
#include <string>
#include <map>
#include <vector>
#include "node.h" // Include Node base class

class Linker {
public:
    // Singleton pattern: Get the single instance of the Linker
    static Linker& getInstance();

    bool initialize();

    // Delete copy constructor and assignment operator to prevent copying
    Linker(const Linker&) = delete;
    Linker& operator=(const Linker&) = delete;

    // Send data from an implicit source to a single destination Node.
    // Returns true on success, false on failure (e.g., target node not found).
    bool send(const std::string& toId, nlohmann::json data);

    // Send data through a sequence of Nodes.
    // The data flows: initial_data -> node1 -> node2 -> ... -> last_node_output.
    // Returns true if the entire stream processing was successful.
    bool sendStream(const std::vector<std::string>& nodeIdsInOrder, nlohmann::json initialData);

    // Send data to multiple destination Nodes simultaneously.
    // Each target node receives the same initial data.
    // Returns true if all sends were successful.
    bool sendMulti(const std::vector<std::string>& toIds, nlohmann::json data);

    // Public for testing purposes (consider making private with a getter in production)
    std::map<std::string, Node*> m_registeredNodes;


    void registerNode(const std::string& nodeId, Node* nodePtr);

private:
    // Private constructor for Singleton pattern
    Linker();
    // Private destructor for Singleton (default is typically fine)
    ~Linker() = default;

    
    // Register a Node with the Linker. The Linker needs to know about all
    // Nodes it might send data to.
    //void registerNode(const std::string& nodeId, Node* nodePtr);
};

#endif // LINKER_H
