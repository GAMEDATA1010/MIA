#ifndef AGENT_H
#define AGENT_H

#include "node.h"
#include <nlohmann/json.hpp>
#include <string>
#include <map> // To store parameters if you want dynamic ones


class InterAgentFormatter: public Node {
public:
    // Constructor to initialize an agent with its personality and desired LLM params
    Agent(const std::string& id,
          const std::string& name);

    // Getters for agent properties
    const std::string& getId() const;
    const std::string& getName() const;

    bool push(nlohmann::json data) override;
    nlohmann::json pull() override;

private:
    const std::string m_name;
};

#endif // AGENT_H
