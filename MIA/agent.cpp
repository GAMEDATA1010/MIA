// agent.cpp
#include "agent.h"

// Constructor implementation
Agent::Agent(const std::string& id,
             const std::string& name,
             const std::string& instructions,
             const LLMParameters& params)
    : m_id(id),
      m_name(name),
      m_instructions(instructions),
      m_llmParams(params) {
    // Constructor body (can be empty if members are initialized in initializer list)
}

// Getter implementations
const std::string& Agent::getId() const {
    return m_id;
}

const std::string& Agent::getName() const {
    return m_name;
}

const std::string& Agent::getInstructions() const {
    return m_instructions;
}

const LLMParameters& Agent::getLLMParameters() const {
    return m_llmParams;
}

