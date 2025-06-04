#ifndef AGENT_H
#define AGENT_H

#include "node.h" // Inherits from Node
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

/**
 * @class Agent
 * @brief Represents an AI agent that can process input and generate responses
 * by interacting with an LLM via the ApiCommunicator.
 *
 * Inherits from Node to integrate into the Linker's communication system.
 */
class Agent: public Node {
public:
    /**
     * @brief Constructor to initialize an agent with its ID, name, and LLM parameters.
     * @param id A unique identifier for this agent instance.
     * @param name A human-readable name for the agent.
     * @param params The LLM parameters specific to this agent's behavior.
     */
    Agent(const std::string& id,
          const std::string& name,
          const LLMParameters& params);

    /**
     * @brief Returns the unique identifier for this agent.
     * @return A const reference to the agent's ID string.
     */
    const std::string& getId() const override;

    /**
     * @brief Returns the human-readable name of this agent.
     * @return A const reference to the agent's name string.
     */
    const std::string& getName() const;

    /**
     * @brief Returns the LLM parameters configured for this agent.
     * @return A const reference to the LLMParameters struct.
     */
    const LLMParameters& getLLMParameters() const;

    /**
     * @brief Pushes data (e.g., user prompt) into the agent for processing.
     * The agent will then prepare an LLM request and send it via the Linker
     * to the ApiCommunicator, then pull the response.
     * @param data The JSON data containing the input for the agent.
     * @return True if the processing and API communication were successful, false otherwise.
     */
    bool push(nlohmann::json data) override;

    /**
     * @brief Pulls the processed data (e.g., LLM generated response) from the agent.
     * @return A JSON object containing the agent's output.
     */
    nlohmann::json pull() override; // Declared as override

private:
    const std::string m_id; // A unique identifier for the agent instance
    const std::string m_name; // A human-readable name for the agent
    const LLMParameters m_llmParams; // Parameters specific to this agent's LLM interaction
    nlohmann::json m_data_in; // Stores the last incoming data
    nlohmann::json m_data_out; // Stores the last outgoing data/response
};

#endif // AGENT_H

