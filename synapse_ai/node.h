#ifndef NODE_H
#define NODE_H

#include <nlohmann/json.hpp>

class Node {
public:
    virtual ~Node() = default; // Added virtual destructor for proper polymorphic deletion
    virtual const std::string& getId() const = 0;
    virtual bool push(nlohmann::json data) = 0;
    virtual nlohmann::json pull() = 0;

protected:
    std::string m_id;
    nlohmann::json m_data_in;
    nlohmann::json m_data_out;
};

#endif
