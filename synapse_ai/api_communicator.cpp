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

// Cleans up cURL resources
void ApiCommunicator::cleanupCurl() {
    if (m_curl) {
        curl_easy_cleanup(m_curl);
        m_curl = nullptr;
    }
    curl_global_cleanup(); // Clean up libcurl's global resources
}

// Static callback function for cURL to write received data
size_t ApiCommunicator::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Initializes the cURL library
bool ApiCommunicator::initCurl() {
    CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (res != CURLE_OK) {
        std::cerr << "curl_global_init() failed: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    m_curl = curl_easy_init();
    if (!m_curl) {
        std::cerr << "curl_easy_init() failed." << std::endl;
        return false;
    }
    return true;
}

// Main method to generate content using the Gemini API
APIResponse ApiCommunicator::generateContent(LLMParameters params, std::string content) {
    APIResponse response;
    // Clear previous response
    m_readBuffer.clear();

    // Reinitialize cURL handle for each request to ensure clean state
    if (m_curl) {
        curl_easy_cleanup(m_curl);
        m_curl = nullptr;
    }
    m_curl = curl_easy_init();
    if (!m_curl) {
        response.success = false;
        response.errorMessage = "Failed to reinitialize cURL handle.";
        return response;
    }

    std::string url = "https://generativelanguage.googleapis.com/v1beta/models/" + params.model + ":generateContent?key=" + m_apiKey;

    nlohmann::json request_body = {
        // "contents" should be a key in the main object, and its value is an array of message objects
        {"contents", nlohmann::json::array({ // Explicitly creating a JSON array for "contents"
            { // This is the first (and likely only) message object in the "contents" array
                {"parts", nlohmann::json::array({ // Explicitly creating a JSON array for "parts"
                    { // This is the first (and likely only) part object in the "parts" array
                        {"text", content}
                    }
                })}
            }
        })},
	// "system instructions" is where the main persona of the LLM is created
        {"system_instruction",
            {
                {"parts", nlohmann::json::array({
                    {
                        {"text", params.instructions}
                    }
                })}
            }
        },
	// "generationConfig" should be a key in the main object, and its value is a parameters object
        {"generationConfig", { // This correctly forms a JSON object for "generationConfig"
            {"temperature", params.temperature},
            {"topP", params.topP},
            {"topK", params.topK},
            {"maxOutputTokens", params.maxOutputTokens}
	}}
    };

    std::string json_payload = request_body.dump();

    curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, json_payload.c_str());
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &m_readBuffer);

    // Set headers
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");
    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(m_curl);
    long http_code = 0;
    curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_slist_free_all(headers); // Free the header list

    response.httpStatusCode = http_code;

    if (res != CURLE_OK) {
        response.success = false;
        response.errorMessage = "cURL error: " + std::string(curl_easy_strerror(res));
    } else {
        response = parseGeminiResponse(m_readBuffer);
        if (!response.success && response.errorMessage.empty()) {
            // Generic error if parse failed and no specific error message from parseGeminiResponse
            response.errorMessage = "API call failed with HTTP status code: " + std::to_string(http_code);
            if (http_code != 200) {
                 response.errorMessage += ". Raw response: " + m_readBuffer;
            }
        }
    }

    logApiCall("N/A", json_payload, m_readBuffer, response); // agentId is not directly available here

    return response;
}

// Node's push method implementation for ApiCommunicator
bool ApiCommunicator::push(nlohmann::json data) {
    m_data_in = data; // Store incoming data

    // Extract parameters from the incoming JSON
    std::string instructions = data.value("instructions", "");
    std::string content = data.value("content", "");
    LLMParameters params;

    // Safely extract LLM parameters, providing defaults or checking existence
    if (data.contains("llm_params")) {
        nlohmann::json llm_params_json = data["llm_params"];
        params.model = llm_params_json.value("model", "gemini-pro");
        params.temperature = llm_params_json.value("temperature", 0.7f);
        params.topP = llm_params_json.value("topP", 0.9f);
        params.topK = llm_params_json.value("topK", 1);
        params.maxOutputTokens = llm_params_json.value("maxOutputTokens", 1024);
        params.maxHistoryTurns = llm_params_json.value("maxHistoryTurns", 5);
    } else {
        // Fallback to default LLMParameters if not provided
        params = {"gemini-pro", 0.7f, 0.9f, 1, 1024, 5, ""};
        std::cerr << "ApiCommunicator Warning: 'llm_params' not found in incoming JSON. Using default LLM parameters." << std::endl;
    }

    // Call the core API generation logic
    APIResponse response = generateContent(params, content);

    // Convert APIResponse to JSON for m_data_out
    m_data_out = nlohmann::json();
    m_data_out["success"] = response.success;
    m_data_out["generated_text"] = response.generatedText;
    m_data_out["error_message"] = response.errorMessage;
    m_data_out["http_status_code"] = response.httpStatusCode;

    return response.success; // Return success status of the API call
}

// Parses the JSON response from the Gemini API
APIResponse ApiCommunicator::parseGeminiResponse(const std::string& jsonResponse) {
    APIResponse response;
    try {
        nlohmann::json parsed_json = nlohmann::json::parse(jsonResponse);

        if (parsed_json.contains("candidates") && parsed_json["candidates"].is_array() && !parsed_json["candidates"].empty()) {
            const auto& candidate = parsed_json["candidates"][0]; // Get the first candidate
            if (candidate.contains("content") && candidate["content"].contains("parts") && candidate["content"]["parts"].is_array() && !candidate["content"]["parts"].empty()) {
                const auto& part = candidate["content"]["parts"][0]; // Get the first part
                if (part.contains("text") && part["text"].is_string()) {
                    response.generatedText = part["text"].get<std::string>();
                    response.success = true;
                }
            }
        } else if (parsed_json.contains("error")) {
            // Handle API-specific errors (e.g., invalid API key, safety issues)
            response.success = false;
            if (parsed_json["error"].contains("message")) {
                response.errorMessage = parsed_json["error"]["message"].get<std::string>();
            } else {
                response.errorMessage = "Unknown API error.";
            }
        }
        // Check for safety blocking if no candidates are present directly (e.g., promptFeedback)
        else if (parsed_json.contains("promptFeedback") && parsed_json["promptFeedback"].contains("blockReason")) {
            response.success = false;
            response.errorMessage = "Prompt blocked due to safety reasons: " + parsed_json["promptFeedback"]["blockReason"].get<std::string>();
        } else {
            response.success = false;
            response.errorMessage = "Unexpected API response format or empty response.";
            // If debugging, include the raw response in the error message for inspection
            if (m_debuggingEnabled) {
                response.errorMessage += "\nRaw Response: " + jsonResponse;
            }
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
    std::cout << "------------------------------------------" << std::endl;
}

// Getter for debugging mode status
bool ApiCommunicator::getDebuggingMode() const {
    return m_debuggingEnabled;
}

// Setter for debugging mode (if you want to enable/disable it dynamically)
void ApiCommunicator::setDebuggingMode(bool enable) {
    m_debuggingEnabled = enable;
    std::cout << "ApiCommunicator Debugging: " << (m_debuggingEnabled ? "Enabled" : "Disabled") << std::endl;
}
