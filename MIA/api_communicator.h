#ifndef API_COMMUNICATOR_H
#define API_COMMUNICATOR_H

#include <string>
#include <curl/curl.h> // For cURL library
#include <nlohmann/json.hpp> // For JSON parsing and building
#include "agent.h"

// Struct to hold the response from the API call
struct APIResponse {
    bool success = false;
    std::string generatedText;
    std::string errorMessage;
    long httpStatusCode = 0;
};

/**
 * @class ApiCommunicator
 * @brief Singleton class responsible for handling communication with external APIs,
 * specifically the Gemini API for LLM content generation.
 *
 * Manages cURL resources and API key, and provides methods to send requests
 * and parse responses.
 */
class ApiCommunicator {
public:
    // Singleton pattern: Get the single instance of ApiCommunicator
    static ApiCommunicator& getInstance();

    // Delete copy constructor and assignment operator to prevent copying
    ApiCommunicator(const ApiCommunicator&) = delete;
    ApiCommunicator& operator=(const ApiCommunicator&) = delete;

    /**
     * @brief Initializes the ApiCommunicator, setting up cURL and retrieving API key.
     * @return True if initialization is successful, false otherwise.
     */
    bool initialize();

    /**
     * @brief Cleans up cURL resources. Called by the destructor.
     */
    void cleanupCurl();

    /**
     * @brief Generates content using the Gemini API.
     * @param params LLM parameters for the generation request.
     * @param content The main text content to send to the LLM.
     * @return An APIResponse struct containing the success status, generated text, or error.
     */
    APIResponse generateContent(LLMParameters params, std::string content);

    /**
     * @brief Pushes incoming data (e.g., LLM request payload) into the communicator.
     * This method acts as the 'push' interface for the ApiCommunicatorNode wrapper.
     * @param data The JSON data containing content and LLM parameters.
     * @return True if the operation was successful, false otherwise.
     */
    bool push(nlohmann::json data);

    /**
     * @brief Pulls the last processed response from the communicator.
     * This method acts as the 'pull' interface for the ApiCommunicatorNode wrapper.
     * @return A JSON object representing the last API response.
     */
    nlohmann::json pull();

    /**
     * @brief Gets the current debugging mode status.
     * @return True if debugging is enabled, false otherwise.
     */
    bool getDebuggingMode() const;

    /**
     * @brief Sets the debugging mode.
     * @param enable True to enable debugging, false to disable.
     */
    void setDebuggingMode(bool enable);

private:
    // Private constructor for Singleton pattern
    ApiCommunicator();
    // Private destructor for Singleton
    ~ApiCommunicator();

    // cURL handle for making HTTP requests
    CURL* m_curl;
    // cURL list for HTTP headers
    struct curl_slist* m_headers;
    // Buffer to store response data from cURL
    static std::string m_readBuffer;
    // Stores the API key
    std::string m_apiKey;
    // Stores the last incoming data (for Node interface)
    nlohmann::json m_data_in;
    // Stores the last outgoing data/response (for Node interface)
    nlohmann::json m_data_out;
    // Flag to control debugging output
    bool m_debuggingEnabled = true; // Default to true for development

    /**
     * @brief Static callback function for cURL to write received data into m_readBuffer.
     */
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

    /**
     * @brief Parses the JSON response received from the Gemini API.
     * @param jsonResponse The raw JSON string received from the API.
     * @return An APIResponse struct populated with parsed data.
     */
    APIResponse parseGeminiResponse(const std::string& jsonResponse);

    /**
     * @brief Logs details of an API call for debugging purposes.
     * @param agentId The ID of the agent making the call (or "N/A").
     * @param requestPayload The JSON payload sent in the request.
     * @param responsePayload The raw JSON response received.
     * @param result The parsed APIResponse struct.
     */
    void logApiCall(const std::string& agentId, const std::string& requestPayload, const std::string& responsePayload, const APIResponse& result) const;
};

#endif // API_COMMUNICATOR_H

