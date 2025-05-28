#ifndef AGENT_MANAGER_H
#define AGENT_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <curl/curl.h> // For HTTP requests
#include <nlohmann/json.hpp> // For JSON parsing and generation
#include <filesystem> // For iterating through directories (C++17)

#include "agent.h" // Include Agent definition

// Structure to hold the result of an API call
struct AgentResponse {
    bool success = false;
    std::string generatedText;
    std::string errorMessage;
    long httpStatusCode = 0; // HTTP status code from the API response
};

// The AgentManager is a singleton class responsible for:
// - Loading agent configurations from JSON files.
// - Managing the cURL library for API communication.
// - Handling the API key.
// - Building and sending requests to the Google Gemini API.
// - Parsing API responses.
// - Logging API calls.
// - NEW: Managing message history per agent.
class AgentManager {
public:
    // Static method to get the single instance of AgentManager (Singleton pattern)
    static AgentManager& getInstance();

    // Delete copy constructor and assignment operator to prevent copying
    AgentManager(const AgentManager&) = delete;
    AgentManager& operator=(const AgentManager&) = delete;

    // Initializes the AgentManager:
    // - Loads base configuration (e.g., API base URL).
    // - Sets the API key from an environment variable.
    // - Initializes the cURL library.
    // - Loads all agent configurations from the specified folder.
    bool initialize(const std::string& baseConfigFilePath, const std::string& agentsFolderPath);

    // Generates content using a specific agent's personality and LLM parameters.
    // It takes the agent's ID and the user's prompt as input.
    // This method will now manage the agent's message history internally.
    AgentResponse generateContent(const std::string& agentId, const std::string& userPrompt);

    // Retrieves a loaded Agent object by its ID. Returns nullptr if not found.
    const Agent* getAgent(const std::string& agentId) const;

    // Destructor to clean up cURL resources
    ~AgentManager();

private:
    // Private constructor to enforce Singleton pattern
    AgentManager();

    // Member variables for API configuration
    std::string m_geminiApiKey;
    std::string m_geminiApiUrl; // Base URL for the Gemini API

    // Default LLM parameters from base_config.json
    std::string m_geminiDefaultModel;
    float m_geminiDefaultTemperature;
    float m_geminiDefaultTopP;
    int m_geminiDefaultTopK;
    int m_geminiDefaultMaxOutputTokens;
    int m_geminiDefaultMaxHistoryTurns; // NEW: Default max history turns

    // Safety filter thresholds from base_config.json
    std::string m_geminiFilterHarassment;
    std::string m_geminiFilterHateSpeech;
    std::string m_geminiFilterSexuallyExplicit;
    std::string m_geminiFilterDangerousContent;

    // cURL related members
    CURL* m_curl; // The cURL easy handle for making HTTP requests
    static std::string m_readBuffer; // Static buffer to store API response data

    // Map to store all loaded Agent instances, keyed by their ID
    std::map<std::string, Agent> m_agents;

    // NEW: Map to store message history for each agent
    // Key: agent ID, Value: vector of JSON message objects (alternating user/model roles)
    std::map<std::string, std::vector<nlohmann::json>> m_agentHistories;

    // Private helper methods

    // Initializes the cURL library.
    bool initCurl();
    // Cleans up the cURL easy handle.
    void cleanupCurl();

    // Callback function for cURL to write received data into m_readBuffer.
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

    // Loads base configuration settings from a JSON file (e.g., API base URL).
    bool loadBaseConfiguration(const std::string& baseConfigFilePath);
    // Loads all agent configurations from JSON files within a specified folder.
    bool loadAgentsFromFolder(const std::string& agentsFolderPath);

    // Builds the JSON request body for the Gemini API call, using the provided message history.
    nlohmann::json buildRequestBody(const LLMParameters& params,
                                    const std::vector<nlohmann::json>& messageHistory,
                                    const std::string& filterHarassment,
                                    const std::string& filterHateSpeech,
                                    const std::string& filterSexuallyExplicit,
                                    const std::string& filterDangerousContent);
    // Parses the JSON response from the Gemini API to extract the generated text.
    AgentResponse parseGeminiResponse(const std::string& jsonResponse);

    // Logs details of an API call (request, response, result).
    void logApiCall(const std::string& agentId, const std::string& requestPayload, const std::string& responsePayload, const AgentResponse& result) const;
};

#endif // AGENT_MANAGER_H

