#include "linker.h" // Include Linker for inter-node communication
#include <iostream>

// Constructor implementation
InterAgentFormatter::Inter(const std::string& id,
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

nlohmann::json Agent::pull() {
        return m_data_out;
}


