// src/api_communicator.cpp (Renamed from agent_manager.cpp)
#include "api_communicator.h"
#include <iostream>
#include <fstream>
#include <sstream> // For std::stringstream
#include <stdexcept> // For std::runtime_error
#include <cstdlib> // For std::getenv
#include <algorithm> // For std::min

// Initialize the static member buffer for cURL callback
std::string ApiCommunicator::m_readBuffer;

// Private constructor implementation (Singleton)
ApiCommunicator::ApiCommunicator() : m_curl(nullptr) {
    // Constructor body, cURL is initialized in initCurl()
}

// Destructor implementation
ApiCommunicator::~ApiCommunicator() {
    cleanupCurl(); // Ensure cURL resources are cleaned up
}

// Static method to get the single instance of ApiCommunicator
ApiCommunicator& ApiCommunicator::getInstance() {
    static ApiCommunicator instance; // Guaranteed to be initialized once and destroyed correctly
    return instance;
}

// Initializes the ApiCommunicator
bool ApiCommunicator::initialize() {
    // 2. Get API Key from environment variable
    const char* apiKeyCStr = std::getenv("GEMINI_API_KEY");
    if (apiKeyCStr == nullptr || std::string(apiKeyCStr).empty()) {
        std::cerr << "ApiCommunicator Error: GEMINI_API_KEY environment variable not set. Please set it before running." << std::endl;
        return false;
    }
    m_apiKey = apiKeyCStr;

    // 3. Initialize cURL
    if (!initCurl()) {
        std::cerr << "ApiCommunicator Error: Failed to initialize cURL." << std::endl;
        return false;
    }

    return true;
}
bool ApiCommunicator::push(nlohmann::json data) {
	m_data_in = data;
	bool success = true;
	return success;
}


// NEW: Sends a request to the API
APIResponse ApiCommunicator::generateContent(std::string instructions, LLMParameters params, std::string content) {
    APIResponse result;
    m_readBuffer.clear(); // Clear buffer for new response

    std::string request_payload = "insert payload equation";
    std::string apiUrlEndpoint = "https://generativelanguage.googleapis.com/v1beta/models/";
    curl_easy_setopt(m_curl, CURLOPT_URL, apiUrlEndpoint.c_str());
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, request_payload.c_str());

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &m_readBuffer);

    CURLcode res = curl_easy_perform(m_curl);
    long http_code = 0;
    curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_slist_free_all(headers); // Clean up headers list

    if (res != CURLE_OK) {
        result.errorMessage = "cURL request failed: " + std::string(curl_easy_strerror(res));
        result.httpStatusCode = http_code;
    } else {
        result = parseGeminiResponse(m_readBuffer);
        result.httpStatusCode = http_code; // Set the actual HTTP code from the response
        if (!result.success && result.errorMessage.empty()) {
             // If parsing failed but no specific error message was set,
             // it might be a generic API error or unexpected format.
             result.errorMessage = "API call failed with HTTP " + std::to_string(http_code) + ". Raw response: " + m_readBuffer.substr(0, std::min((size_t)500, m_readBuffer.length())) + "...";
        }
    }
    
    // Log the API call
    
    return result;
}

// Private helper methods implementations

// Initializes the cURL library.
bool ApiCommunicator::initCurl() {
    curl_global_init(CURL_GLOBAL_DEFAULT); // Initialize libcurl
    m_curl = curl_easy_init();
    if (!m_curl) {
        std::cerr << "Error: Could not initialize cURL easy handle." << std::endl;
        return false;
    }
    return true;
}

// Cleans up the cURL easy handle.
void ApiCommunicator::cleanupCurl() {
    if (m_curl) {
        curl_easy_cleanup(m_curl);
        m_curl = nullptr;
    }
    curl_global_cleanup(); // Clean up libcurl
}

// Callback function for cURL to write received data into m_readBuffer.
size_t ApiCommunicator::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Parses the JSON response from the Gemini API to extract the generated text.
APIResponse ApiCommunicator::parseGeminiResponse(const std::string& jsonResponse) {
    APIResponse response;
    try {
        nlohmann::json parsed_json = nlohmann::json::parse(jsonResponse);

        if (parsed_json.contains("error")) {
            response.success = false;
            response.errorMessage = parsed_json["error"]["message"].get<std::string>();
            return response;
        }

        if (parsed_json.contains("candidates") && parsed_json["candidates"].is_array() && !parsed_json["candidates"].empty()) {
            const auto& first_candidate = parsed_json["candidates"][0];
            if (first_candidate.contains("content") && first_candidate["content"].contains("parts") && first_candidate["content"]["parts"].is_array() && !first_candidate["content"]["parts"].empty()) {
                const auto& first_part = first_candidate["content"]["parts"][0];
                if (first_part.contains("text")) {
                    response.generatedText = first_part["text"].get<std::string>();
                    response.success = true;
                }
            } else if (first_candidate.contains("finishReason") && first_candidate["finishReason"].get<std::string>() == "SAFETY") {
                response.success = false;
                response.errorMessage = "Response blocked due to safety reasons.";
            }
        } else if (parsed_json.contains("promptFeedback") && parsed_json["promptFeedback"].contains("blockReason")) {
            response.success = false;
            response.errorMessage = "Prompt blocked due to safety reasons: " + parsed_json["promptFeedback"]["blockReason"].get<std::string>();
        } else {
            response.success = false;
            response.errorMessage = "Unexpected API response format.";
        }
    } catch (const nlohmann::json::exception& e) {
        response.success = false;
        response.errorMessage = "JSON parsing error: " + std::string(e.what());
    }
    return response;
}

// Logs details of an API call (request, response, result).
void ApiCommunicator::logApiCall(const std::string& agentId, const std::string& requestPayload, const std::string& responsePayload, const APIResponse& result) const {
    if (!m_debuggingEnabled) {
        return;
    }

    std::cout << "\n--- API Call Log for Agent: " << agentId << " ---" << std::endl;
    std::cout << "Request:\n" << requestPayload << std::endl;
    std::cout << "Response:\n" << responsePayload << std::endl;
    std::cout << "Status: " << (result.success ? "SUCCESS" : "FAILED") << std::endl;
    if (!result.success) {
        std::cout << "Error Message: " << result.errorMessage << std::endl;
        std::cout << "HTTP Status Code: " << result.httpStatusCode << std::endl;
    }
    std::cout << "Generated Text (if any):\n" << result.generatedText << std::endl;
    std::cout << "--- End API Call Log ---\n" << std::endl;
}
