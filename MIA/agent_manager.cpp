// agent_manager.cpp
#include "agent_manager.h"
#include <iostream>
#include <fstream>
#include <sstream> // For std::stringstream
#include <stdexcept> // For std::runtime_error
#include <cstdlib> // For std::getenv

// Initialize the static member buffer for cURL callback
std::string AgentManager::m_readBuffer;

// Private constructor implementation (Singleton)
AgentManager::AgentManager() : m_curl(nullptr) {
    // Constructor body, cURL is initialized in initCurl()
}

// Destructor implementation
AgentManager::~AgentManager() {
    cleanupCurl(); // Ensure cURL resources are cleaned up
}

// Static method to get the single instance of AgentManager
AgentManager& AgentManager::getInstance() {
    static AgentManager instance; // Guaranteed to be initialized once and destroyed correctly
    return instance;
}

// Initializes the AgentManager
bool AgentManager::initialize(const std::string& baseConfigFilePath, const std::string& agentsFolderPath) {
    // 1. Load base configuration (API URL)
    if (!loadBaseConfiguration(baseConfigFilePath)) {
        std::cerr << "AgentManager Error: Failed to load base configuration from " << baseConfigFilePath << std::endl;
        return false;
    }

    // 2. Get API Key from environment variable
    const char* apiKeyCStr = std::getenv("GEMINI_API_KEY");
    if (apiKeyCStr == nullptr) {
        std::cerr << "AgentManager Error: GEMINI_API_KEY environment variable not set. Please set it before running." << std::endl;
        return false;
    }
    m_geminiApiKey = apiKeyCStr;

    // 3. Initialize cURL
    if (!initCurl()) {
        std::cerr << "AgentManager Error: Failed to initialize cURL." << std::endl;
        return false;
    }

    // 4. Load all agents from the specified folder
    if (!loadAgentsFromFolder(agentsFolderPath)) {
        std::cerr << "AgentManager Error: Failed to load agents from folder: " << agentsFolderPath << std::endl;
        return false;
    }

    std::cout << "AgentManager: Initialized successfully. Loaded " << m_agents.size() << " agents." << std::endl;
    return true;
}

// Loads base configuration (e.g., API base URL) from a JSON file
bool AgentManager::loadBaseConfiguration(const std::string& baseConfigFilePath) {
    std::ifstream file(baseConfigFilePath);
    if (!file.is_open()) {
        std::cerr << "AgentManager Error: Could not open base configuration file: " << baseConfigFilePath << std::endl;
        return false;
    }
    try {
        nlohmann::json config_json = nlohmann::json::parse(file);
        if (config_json.contains("api_url")) {
            m_geminiApiUrl = config_json["api_url"].get<std::string>();
        } else {
            std::cerr << "AgentManager Error: 'api_url' not found in base config file: " << baseConfigFilePath << std::endl;
            return false;
        }
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "AgentManager Error: Parsing base config JSON failed: " << e.what() << std::endl;
        return false;
    }
    return true;
}

// Loads all agent configurations from JSON files within a specified folder
bool AgentManager::loadAgentsFromFolder(const std::string& agentsFolderPath) {
    m_agents.clear(); // Clear any previously loaded agents
    namespace fs = std::filesystem; // Use C++17 filesystem

    if (!fs::exists(agentsFolderPath) || !fs::is_directory(agentsFolderPath)) {
        std::cerr << "AgentManager Error: Agents folder not found or is not a directory: " << agentsFolderPath << std::endl;
        return false;
    }

    // Iterate through each item in the agents folder
    for (const auto& entry : fs::directory_iterator(agentsFolderPath)) {
        // Check if it's a regular file and has a .json extension
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            std::ifstream agentFile(entry.path());
            if (!agentFile.is_open()) {
                std::cerr << "AgentManager Warning: Could not open agent config file: " << entry.path() << std::endl;
                continue; // Skip to the next file
            }

            try {
                nlohmann::json agent_json = nlohmann::json::parse(agentFile);

                // Extract required agent details
                std::string id = agent_json.at("id").get<std::string>();
                std::string name = agent_json.at("name").get<std::string>();
                std::string instructions = agent_json.at("instructions").get<std::string>();

                // Extract LLM parameters, providing default values if not present
                LLMParameters params;
                const auto& params_json = agent_json.at("parameters"); // 'at' throws if not found
                params.model = params_json.value("model", "gemini-2.0-flash"); // Default model
                params.temperature = params_json.value("temperature", 0.7f);
                params.topP = params_json.value("top_p", 1.0f);
                params.topK = params_json.value("top_k", 0);
                params.maxOutputTokens = params_json.value("max_output_tokens", 800);

                // Create Agent object and store it in the map
                m_agents.emplace(id, Agent(id, name, instructions, params));
                std::cout << "AgentManager: Loaded agent '" << name << "' (ID: " << id << ") from " << entry.path().filename() << std::endl;

            } catch (const nlohmann::json::exception& e) {
                std::cerr << "AgentManager Error: Parsing agent JSON file " << entry.path() << " failed: " << e.what() << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "AgentManager Error: Unexpected error loading agent " << entry.path() << ": " << e.what() << std::endl;
            }
        }
    }
    return true;
}

// Retrieves a loaded Agent object by its ID
const Agent* AgentManager::getAgent(const std::string& agentId) const {
    auto it = m_agents.find(agentId);
    if (it != m_agents.end()) {
        return &(it->second); // Return pointer to the Agent object
    }
    return nullptr; // Agent not found
}

// Initializes the cURL library
bool AgentManager::initCurl() {
    CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (res != CURLE_OK) {
        std::cerr << "AgentManager Error: curl_global_init() failed: " << curl_easy_strerror(res) << std::endl;
        return false;
    }
    m_curl = curl_easy_init();
    if (!m_curl) {
        std::cerr << "AgentManager Error: curl_easy_init() failed." << std::endl;
        return false;
    }
    return true;
}

// Cleans up the cURL easy handle
void AgentManager::cleanupCurl() {
    if (m_curl) {
        curl_easy_cleanup(m_curl);
        m_curl = nullptr;
    }
    curl_global_cleanup();
}

// Static callback function for cURL to write received data
size_t AgentManager::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Builds the JSON request body for the Gemini API call
nlohmann::json AgentManager::buildRequestBody(const LLMParameters& params, const std::string& instructions, const std::string& userPrompt) {
    nlohmann::json request_body;

    // Combine instructions and user prompt into a single user message
    // Gemini API often expects a conversational turn structure
    request_body["contents"] = {
        {
            {"role", "user"},
            {"parts", {
                {"text", instructions + "\n\n" + userPrompt} // Instructions typically prepended to the first user message
            }}
        }
    };

    // Add generation configuration parameters
    request_body["generationConfig"] = {
        {"temperature", params.temperature},
        {"topP", params.topP},
        {"topK", params.topK},
        {"maxOutputTokens", params.maxOutputTokens}
    };

    // Optionally add safety settings if needed
    // request_body["safetySettings"] = { ... };

    return request_body;
}

// Parses the JSON response from the Gemini API
AgentResponse AgentManager::parseGeminiResponse(const std::string& jsonResponse) {
    AgentResponse result;
    try {
        nlohmann::json response_json = nlohmann::json::parse(jsonResponse);

        if (response_json.contains("candidates") && response_json["candidates"].is_array() && !response_json["candidates"].empty()) {
            const auto& first_candidate = response_json["candidates"][0];
            if (first_candidate.contains("content") && first_candidate["content"].contains("parts") && first_candidate["content"]["parts"].is_array() && !first_candidate["content"]["parts"].empty()) {
                const auto& first_part = first_candidate["content"]["parts"][0];
                if (first_part.contains("text")) {
                    result.generatedText = first_part["text"].get<std::string>();
                    result.success = true;
                } else {
                    result.errorMessage = "Response part does not contain 'text'.";
                }
            } else {
                result.errorMessage = "Response candidate does not contain valid 'content' or 'parts'.";
            }
        } else if (response_json.contains("error")) {
            result.errorMessage = response_json["error"].value("message", "Unknown API error.");
            if (response_json["error"].contains("code")) {
                result.httpStatusCode = response_json["error"]["code"].get<long>();
            }
        } else {
            result.errorMessage = "Unexpected API response format.";
        }
    } catch (const nlohmann::json::exception& e) {
        result.errorMessage = "JSON parsing error: " + std::string(e.what());
    } catch (const std::exception& e) {
        result.errorMessage = "Unexpected error during response parsing: " + std::string(e.what());
    }
    return result;
}

// Logs details of an API call
void AgentManager::logApiCall(const std::string& agentId, const std::string& requestPayload, const std::string& responsePayload, const AgentResponse& result) const {
    std::cout << "\n--- API Call Log for Agent: " << agentId << " ---" << std::endl;
    std::cout << "Request Payload: " << requestPayload << std::endl;
    std::cout << "Response Payload: " << responsePayload << std::endl;
    std::cout << "Success: " << (result.success ? "Yes" : "No") << std::endl;
    if (!result.success) {
        std::cout << "Error Message: " << result.errorMessage << std::endl;
        std::cout << "HTTP Status Code: " << result.httpStatusCode << std::endl;
    }
    std::cout << "--------------------------------------\n" << std::endl;
}


// Main method to generate content using a specific agent's parameters
AgentResponse AgentManager::generateContent(const std::string& agentId, const std::string& userPrompt) {
    AgentResponse result;
    const Agent* agent = getAgent(agentId);
    if (!agent) {
        result.errorMessage = "Error: Agent with ID '" + agentId + "' not found.";
        return result;
    }

    // Build the request URL
    std::string requestUrl = m_geminiApiUrl + agent->getLLMParameters().model + ":generateContent?key=" + m_geminiApiKey;

    // Build the JSON request body
    nlohmann::json request_body_json = buildRequestBody(agent->getLLMParameters(), agent->getInstructions(), userPrompt);
    std::string request_payload = request_body_json.dump();

    // Reset read buffer for new response
    m_readBuffer.clear();

    // Set cURL options
    curl_easy_setopt(m_curl, CURLOPT_URL, requestUrl.c_str());
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, request_payload.c_str()); // Set POST data

    // Set headers
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);

    // Set write callback function to capture response
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &m_readBuffer);

    // Perform the request
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
             result.errorMessage = "API call failed with HTTP " + std::to_string(http_code) + ". Raw response: " + m_readBuffer.substr(0, 200) + "...";
        }
    }

    // Log the API call details
    logApiCall(agentId, request_payload, m_readBuffer, result);

    return result;
}

