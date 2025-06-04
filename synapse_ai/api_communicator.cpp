// src/api_communicator.cpp (Renamed from agent_manager.cpp)
#include "api_communicator.h"
#include <iostream>
#include <fstream>
#include <sstream> // For std::stringstream
#include <stdexcept> // For std::runtime_error
#include <cstdlib> // For std::getenv
#include <algorithm> // For std::min
#include <chrono>

// Initialize the static member buffer for cURL callback
std::string ApiCommunicator::m_readBuffer;

// Private constructor implementation (Singleton)
ApiCommunicator::ApiCommunicator() : m_curl(nullptr), m_headers(nullptr) {
    // m_curl is initialized in initialize()
    // curl_global_init() should be called only once globally, handled in initialize()
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
    // 1. Global cURL initialization (call only once)
    CURLcode global_res = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (global_res != CURLE_OK) {
        std::cerr << "curl_global_init() failed: " << curl_easy_strerror(global_res) << std::endl;
        return false;
    }

    // 2. Initialize cURL easy handle (call once, reuse thereafter)
    m_curl = curl_easy_init();
    if (!m_curl) {
        std::cerr << "curl_easy_init() failed." << std::endl;
        return false;
    }

    // 3. Get API Key from environment variable
    const char* apiKeyCStr = std::getenv("GEMINI_API_KEY");
    if (apiKeyCStr == nullptr || std::string(apiKeyCStr).empty()) {
        std::cerr << "ApiCommunicator Error: GEMINI_API_KEY environment variable not set. Please set it before running." << std::endl;
        return false;
    }
    m_apiKey = apiKeyCStr;

    // 4. Set static headers ONCE for the handle
    // These headers will be reused for all requests.
    // If curl_easy_reset() is used, these will need to be re-applied.
    m_headers = curl_slist_append(m_headers, "Content-Type: application/json");
    m_headers = curl_slist_append(m_headers, "Accept: application/json");
    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, m_headers);

    // Set common options here that don't change per request
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &m_readBuffer);
    // Consider adding a timeout
    // curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, 30L); // 30 second timeout

    return true;
}

// Cleans up cURL resources
void ApiCommunicator::cleanupCurl() {
    if (m_headers) {
        curl_slist_free_all(m_headers); // Free headers
        m_headers = nullptr;
    }
    if (m_curl) {
        curl_easy_cleanup(m_curl); // Clean up CURL handle
        m_curl = nullptr;
    }
    curl_global_cleanup(); // Clean up libcurl's global resources
}

// Static callback function for cURL to write received data
size_t ApiCommunicator::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Main method to generate content using the Gemini API
APIResponse ApiCommunicator::generateContent(LLMParameters params, std::string content) {
    APIResponse response;
    m_readBuffer.clear(); // Clear previous response data

    // Reset the cURL handle to clear previous options, but keep the handle itself.
    // This is more efficient than cleanup/re-init for each request.
    curl_easy_reset(m_curl);

    // Re-apply common options and headers after reset
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &m_readBuffer);
    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, m_headers); // Re-apply the headers!

    std::string url = "https://generativelanguage.googleapis.com/v1beta/models/" + params.model + ":generateContent?key=" + m_apiKey;

    nlohmann::json request_body = {
        {"contents", nlohmann::json::array({
            {
                {"parts", nlohmann::json::array({
                    {
                        {"text", content}
                    }
                })}
            }
        })},
        {"system_instruction",
            {
                {"parts", nlohmann::json::array({
                    {
                        {"text", params.instructions}
                    }
                })}
            }
        },
        {"generationConfig", {
            {"temperature", params.temperature},
            {"topP", params.topP},
            {"topK", params.topK},
            {"maxOutputTokens", params.maxOutputTokens}
        }}
    };

    std::string json_payload = request_body.dump();

    // Set URL and POST data for this specific request
    curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, json_payload.c_str());
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, json_payload.length()); // Important for POST requests

    // --- Timing measurements ---
    auto start = std::chrono::high_resolution_clock::now();
    CURLcode res = curl_easy_perform(m_curl);
    auto curl_perform_complete = std::chrono::high_resolution_clock::now();
    // --- End Timing measurements ---

    long http_code = 0;
    curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &http_code);
    
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

    auto parsing_complete = std::chrono::high_resolution_clock::now();

    // Output timing for debugging performance
    std::cout << "curl_perform duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(curl_perform_complete - start).count() << "ms" << std::endl;
    std::cout << "parsing duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(parsing_complete - curl_perform_complete).count() << "ms" << std::endl;

    logApiCall("N/A", json_payload, m_readBuffer, response); // agentId is not directly available here

    return response;
}

// Node's push method implementation for ApiCommunicator (used by ApiCommunicatorNode wrapper)
bool ApiCommunicator::push(nlohmann::json data) {
    m_data_in = data; // Store incoming data

    // Extract parameters from the incoming JSON
    std::string content = data.value("content", "");
    LLMParameters params;

    // Safely extract LLM parameters, providing defaults or checking existence
    if (data.contains("llm_params")) {
        nlohmann::json llm_params_json = data["llm_params"];
        params.model = llm_params_json.value("model", "gemini-pro");
        params.instructions = llm_params_json.value("instructions","");
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
    std::cout << "generating content..." << std::endl;
    // Call the core API generation logic
    APIResponse response = generateContent(params, content);
    std::cout << "content generated!" << std::endl;
    // Convert APIResponse to JSON for m_data_out
    m_data_out = nlohmann::json();
    m_data_out["success"] = response.success;
    m_data_out["generated_text"] = response.generatedText;
    m_data_out["error_message"] = response.errorMessage;
    m_data_out["http_status_code"] = response.httpStatusCode;

    std::cout << m_data_out << std::endl;

    return response.success; // Return success status of the API call
}

nlohmann::json ApiCommunicator::pull() {
    return m_data_out;
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

