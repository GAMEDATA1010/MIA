// include/api_communicator.h (Renamed from agent_manager.h)
#ifndef API_COMMUNICATOR_H
#define API_COMMUNICATOR_H

#include <string>
#include <vector>
#include <map>
#include <curl/curl.h> // For HTTP requests
#include <nlohmann/json.hpp> // For JSON parsing and generation
#include <filesystem> // For iterating through directories (C++17)
#include <functional> // For std::function
#include "node.h"
#include "agent.h"

// Structure to hold the result of an API call (renamed from AgentResponse)
struct APIResponse {
    bool success = false;
    std::string generatedText;
    std::string errorMessage;
    long httpStatusCode = 0; // HTTP status code from the API response
};

// The ApiCommunicator is a singleton class responsible for:
// - Managing the cURL library for API communication.
// - Handling the API key.
// - Sending requests to the Google Gemini API.
// - Parsing API responses.
// - Logging API calls.
class ApiCommunicator: public Node {
public:
    // Static method to get the single instance of ApiCommunicator (Singleton pattern)
    static ApiCommunicator& getInstance();

    // Delete copy constructor and assignment operator to prevent copying
    ApiCommunicator(const ApiCommunicator&) = delete;
    ApiCommunicator& operator=(const ApiCommunicator&) = delete;

    // Initializes the ApiCommunicator:
    // - Loads base configuration (e.g., API base URL).
    // - Sets the API key from an environment variable.
    // - Initializes the cURL library.
    bool initialize(); // No longer loads agents

    // NEW: Sends a request to the API and returns the response.
    APIResponse generateContent(std::string instructions, LLMParameters params, std::string content);

    bool push(nlohmann::json data) override;

    // Existing: Debugging settings getter
    bool getDebuggingMode() const { return m_debuggingEnabled; }

private:
    ApiCommunicator();
    ~ApiCommunicator();

    std::string m_apiKey;
    bool m_debuggingEnabled = false; // Flag to enable/disable debugging logs

    CURL* m_curl; // The cURL easy handle for making HTTP requests
    static std::string m_readBuffer; // Static buffer to store API response data

    // Private helper methods

    // Initializes the cURL library.
    bool initCurl();
    // Cleans up the cURL easy handle.
    void cleanupCurl();

    // Callback function for cURL to write received data into m_readBuffer.
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

    // Parses the JSON response from the Gemini API to extract the generated text.
    APIResponse parseGeminiResponse(const std::string& jsonResponse);

    // Logs details of an API call (request, response, result).
    void logApiCall(const std::string& agentId, const std::string& requestPayload, const std::string& responsePayload, const APIResponse& result) const;
};

#endif // API_COMMUNICATOR_H
