// agent.cpp
#include "agent.h"
#include "api_communicator.h" // Still need ApiCommunicator header for types like LLMParameters
#include "linker.h" // Include Linker for inter-node communication
#include <iostream>

// Constructor implementation
Agent::Agent(const std::string& id,
             const std::string& name,
             const LLMParameters& params)
    : m_id(id),
      m_name(name),
      m_llmParams(params) {
    // Constructor body
}

// Getter implementations
const std::string& Agent::getId() const {
    return m_id;
}

const std::string& Agent::getName() const {
    return m_name;
}

const LLMParameters& Agent::getLLMParameters() const {
    return m_llmParams;
}

nlohmann::json Agent::pull() {
	return m_data_out;
}

// Agent's push method: Receives data (e.g., user prompt), prepares LLM request,
// sends to ApiCommunicator via Linker, then pulls response.
bool Agent::push(nlohmann::json data) {
    m_data_in = data; // Store the incoming data (e.g., user prompt JSON)

    // Ensure the incoming data has a "content" field (the actual user prompt)
    if (!m_data_in.contains("content") || !m_data_in["content"].is_string()) {
        std::cerr << "Agent Error: Incoming data to push() does not contain a 'content' string." << std::endl;
        // Set an error message in m_data_out so pull() can retrieve it
        m_data_out = {{"success", false}, {"error_message", "Invalid input format to Agent push()."}};
        return false;
    }

    std::string user_content = m_data_in["content"].get<std::string>();

    // 1. Prepare the JSON payload for ApiCommunicator
    // This payload contains all necessary info for the LLM API call
    nlohmann::json llm_request_payload = {
        {"content", user_content},
        {"llm_params", {
            {"model", m_llmParams.model},
            {"instructions", m_llmParams.instructions},
	    {"temperature", m_llmParams.temperature},
            {"topP", m_llmParams.topP},
            {"topK", m_llmParams.topK},
            {"maxOutputTokens", m_llmParams.maxOutputTokens},
            {"maxHistoryTurns", m_llmParams.maxHistoryTurns}
        }}
    };

    std::cout << "Agent '" << m_id << "': Sending LLM request to ApiCommunicator via Linker." << std::endl;

    // 2. Send the LLM request payload to the ApiCommunicator Node via the Linker
    bool send_success = Linker::getInstance().send("api_communicator", llm_request_payload);

    std::cout << llm_request_payload << std::endl;
    if (!send_success) {
        std::cerr << "Agent '" << m_id << "': Failed to send LLM request to ApiCommunicator via Linker." << std::endl;
        m_data_out = {{"success", false}, {"error_message", "Failed to communicate with API."}};
        return false;
    }

    // 3. Pull the response from ApiCommunicator (assuming it has processed the request)
    // In a truly asynchronous system, this would involve a callback or event.
    // For synchronous push/pull Node model, we immediately pull.
    m_data_out = ApiCommunicator::getInstance().pull();

    if (m_data_out.contains("success") && m_data_out["success"].is_boolean() && m_data_out["success"].get<bool>()) {
        std::cout << "Agent '" << m_id << "': Successfully received LLM response." << std::endl;
        return true;
    } else {
        std::cerr << "Agent '" << m_id << "': LLM response indicates failure or unexpected format." << std::endl;
        std::cerr << "Raw API Communicator response: " << m_data_out.dump(2) << std::endl;
        // m_data_out already contains error details from ApiCommunicator
        return false;
    }
}

// Node's pull method is inherited and implemented in node.cpp
// It simply returns m_data_out
