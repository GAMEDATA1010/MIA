#ifndef AGENT_H
#define AGENT_H

#include <string>
#include <map> // To store parameters if you want dynamic ones

// Struct to hold LLM parameters for an agent
struct LLMParameters {
    std::string model;
    float temperature;
    float topP;
    int topK;
    int maxOutputTokens;
    // Add other relevant parameters as needed by the LLM API
};

class Agent {
public:
    // Constructor to initialize an agent with its personality and desired LLM params
    Agent(const std::string& id,
          const std::string& name,
          const std::string& instructions,
          const LLMParameters& params);

    // Getters for agent properties
    const std::string& getId() const;
    const std::string& getName() const;
    const std::string& getInstructions() const;
    const LLMParameters& getLLMParameters() const; // Provides access to the parameters

private:
    const std::string m_id; // A unique identifier for the agent type
    const std::string m_name;
    const std::string m_instructions;
    const LLMParameters m_llmParams; // Parameters specific to this agent's "personality"
};

#endif // AGENT_H
