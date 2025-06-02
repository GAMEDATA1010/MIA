#ifndef AGENT_H
#define AGENT_H

#include "node.h"
#include <nlohmann/json.hpp>
#include <string>
#include <map> // To store parameters if you want dynamic ones

// Struct to hold LLM parameters for an agent
struct LLMParameters {
    std::string model;
    float temperature;
    float topP;
    int topK;
    int maxOutputTokens;
    int maxHistoryTurns;
    std::string instructions;
    // Add other relevant parameters as needed by the LLM API
};

class Agent: public Node {
public:
    // Constructor to initialize an agent with its personality and desired LLM params
    Agent(const std::string& id,
          const std::string& name,
          const LLMParameters& params);

    // Getters for agent properties
    const std::string& getId() const;
    const std::string& getName() const;
    const LLMParameters& getLLMParameters() const; // Provides access to the parameters

    bool push(nlohmann::json data) override;
    bool requestContentGeneration();

private:
    const std::string m_id; // A unique identifier for the agent type
    const std::string m_name;
    const LLMParameters m_llmParams; // Parameters specific to this agent
};

#endif // AGENT_H
