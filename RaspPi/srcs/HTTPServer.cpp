#include "../includes/HTTPServer.h"
#include "../includes/Logger.h"
#include "../includes/ConfigManager.h"

#include <sstream>
#ifdef RASPBERRY_PI
#include <json/json.h>
#endif
#include <ctime>

HTTPServer::~HTTPServer()
{
    Stop();
}

bool HTTPServer::Start(int serverPort, const std::string& serverAddress)
{
    if (running)
    {
        LOG_WARNING("HTTP server already running");
        return true;
    }
    
    port = serverPort;
    address = serverAddress;
    
    ConfigManager& config = ConfigManager::GetInstance();
    std::string apiKey = config.GetString("security.api_key", "");
    bool enableAuth = config.GetBool("security.enable_auth", false);
    
    if (enableAuth && !apiKey.empty())
    {
        AddAPIKey(apiKey, "default_client");
        LOG_INFO("API authentication enabled");
    }
    
#ifdef RASPBERRY_PI
    daemon = MHD_start_daemon(
        MHD_USE_SELECT_INTERNALLY,
        port,
        NULL, NULL,
        &HTTPServer::handleRequest, this,
        MHD_OPTION_NOTIFY_COMPLETED, NULL, NULL,
        MHD_OPTION_END);
#endif
    
    if (!daemon)
    {
        LOG_ERROR("Failed to start HTTP server on " + address + ":" + std::to_string(port));
        return false;
    }
    
    running = true;
    LOG_INFO("HTTP server started on http://" + address + 
             ":" + std::to_string(port));
    LOG_INFO("Available endpoints:");
    LOG_INFO("  GET  /on       - Turn relay ON");
    LOG_INFO("  GET  /off      - Turn relay OFF");
    LOG_INFO("  GET  /toggle   - Toggle relay state");
    LOG_INFO("  GET  /status   - Get current status");
    LOG_INFO("  GET  /health   - Health check");
    
    return true;
}

void HTTPServer::Stop()
{
    if (daemon)
    {
#ifdef RASPBERRY_PI
        MHD_stop_daemon(daemon);
#endif
        daemon = nullptr;
        running = false;
        LOG_INFO("HTTP server stopped");
    }
}

int HTTPServer::HandleRequest(void* cls, struct MHD_Connection* connection,
                            const char* url, const char* method,
                            const char* version, const char* upload_data,
                            size_t* upload_data_size, void** con_cls)
{
    HTTPServer* server = static_cast<HTTPServer*>(cls);
    return server->ProcessRequest(connection, url, method);
}

int HTTPServer::ProcessRequest(struct MHD_Connection* connection, const std::string& url, const std::string& method)
{
    std::string clientIP = GetClientIP(connection);
    int ret = 0;

    if (!CheckAuthentication(connection))
    {
        LogRequest(clientIP, method, url, 401);
#ifdef RASPBERRY_PI
        Json::Value response;
        response["status"] = "error";
        response["message"] = "Unauthorized";
        
        std::string responseStr = response.toStyledString();
        
        struct MHD_Response* mhdResponse = MHD_create_response_from_buffer(
            responseStr.length(),
            (void*)responseStr.c_str(),
            MHD_RESPMEM_MUST_COPY);
        
        MHD_add_response_header(mhdResponse, "Content-Type", "application/json");
        ret = MHD_queue_response(connection, 401, mhdResponse);
        MHD_destroy_response(mhdResponse);
#endif
        return ret;
    }
    
    std::string responseStr;
    int responseCode = 200;
    
#ifdef RASPBERRY_PI
    Json::Value response;
    
    if (method == "GET")
    {
        if (url == "/on")
        {
            if (relay.TurnOn())
            {
                response["status"] = "success";
                response["message"] = "Relay turned ON";
                response["state"] = "on";
            }
            else
            {
                response["status"] = "error";
                response["message"] = "Failed to turn relay ON";
                responseCode = 500;
            }
        }
        else if (url == "/off")
        {
            if (relay.TurnOff())
            {
                response["status"] = "success";
                response["message"] = "Relay turned OFF";
                response["state"] = "off";
            }
            else
            {
                response["status"] = "error";
                response["message"] = "Failed to turn relay OFF";
                responseCode = 500;
            }
        }
        else if (url == "/toggle")
        {
            if (relay.Toggle())
            {
                response["status"] = "success";
                response["message"] = "Relay toggled";
                response["state"] = relay.IsOn() ? "on" : "off";
            }
            else
            {
                response["status"] = "error";
                response["message"] = "Failed to toggle relay";
                responseCode = 500;
            }
        }
        else if (url == "/power")
        {
            responseStr = handlePowerRequest();
        }
        else if (url == "/energy")
        {
            responseStr = handleEnergyRequest();
        }
        else if (url.find("/stats/") == 0)
        {
            std::string period = url.substr(7);
            responseStr = handleStatsRequest(period);
        }
        else if (url == "/sensor/config")
        {
            responseStr = handleSensorConfigRequest();
        }
        else if (url.find("/calibrate") == 0)
        {
            responseStr = handleCalibrationRequest(url);
        }
        else if (url == "/status")
        {
            response["status"] = "success";
            response["state"] = relay.IsOn() ? "on" : "off";
            response["uptime"] = static_cast<int>(std::time(nullptr));
        }
        else if (url == "/health")
        {
            response["status"] = "alive";
            response["timestamp"] = static_cast<int>(std::time(nullptr));
        }
        else
        {
            response["status"] = "error";
            response["message"] = "Endpoint not found";
            responseCode = 404;
        }
    }
    else
    {
        response["status"] = "error";
        response["message"] = "Method not allowed";
        responseCode = 405;
    }
    
    responseStr = response.toStyledString();
    
    LogRequest(clientIP, method, url, responseCode);
    
    struct MHD_Response* mhdResponse = MHD_create_response_from_buffer(
        responseStr.length(),
        (void*)responseStr.c_str(),
        MHD_RESPMEM_MUST_COPY);
    
    MHD_add_response_header(mhdResponse, "Content-Type", "application/json");
    MHD_add_response_header(mhdResponse, "Access-Control-Allow-Origin", "*");
    MHD_add_response_header(mhdResponse, "Access-Control-Allow-Methods", "GET, OPTIONS");
    MHD_add_response_header(mhdResponse, "Access-Control-Allow-Headers", "Content-Type, X-API-Key");
    
    ret = MHD_queue_response(connection, responseCode, mhdResponse);
    MHD_destroy_response(mhdResponse);
#endif
    return ret;
}

bool HTTPServer::CheckAuthentication(struct MHD_Connection* connection)
{
    ConfigManager& config = ConfigManager::GetInstance();
    bool enableAuth = config.GetBool("security.enable_auth", false);
    
    if (!enableAuth || apiKeys.empty())
        return true;
    
    const char* apiKey = nullptr;
#ifdef RASPBERRY_PI
    apiKey = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "X-API-Key");
#endif

    if (!apiKey)
        return false;
    
    return apiKeys.find(apiKey) != apiKeys.end();
}

std::string HTTPServer::GetClientIP(struct MHD_Connection* connection)
{
#ifdef RASPBERRY_PI
    const union MHD_ConnectionInfo* info = MHD_get_connection_info(connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);
    
    if (info && info->client_addr)
    {
        char buffer[INET6_ADDRSTRLEN];
        if (info->client_addr->sa_family == AF_INET)
        {
            struct sockaddr_in* addr = (struct sockaddr_in*)info->client_addr;
            inet_ntop(AF_INET, &addr->sin_addr, buffer, sizeof(buffer));
        }
        else if (info->client_addr->sa_family == AF_INET6)
        {
            struct sockaddr_in6* addr = (struct sockaddr_in6*)info->client_addr;
            inet_ntop(AF_INET6, &addr->sin6_addr, buffer, sizeof(buffer));
        }
        else
            return "unknown";

        return std::string(buffer);
    }
#endif    
    return "unknown";
}

void HTTPServer::LogRequest(const std::string& clientIP, 
                          const std::string& method, 
                          const std::string& url, 
                          int responseCode)
{
    LOG_INFO("[" + clientIP + "] " + method + " " + url + " -> " + std::to_string(responseCode));
}

void HTTPServer::AddAPIKey(const std::string& key, const std::string& clientName)
{
    if (!key.empty())
    {
        apiKeys[key] = clientName.empty() ? "unnamed_client" : clientName;
        LOG_INFO("Added API key for client: " + apiKeys[key]);
    }
}

std::string HTTPServer::handlePowerRequest()
{
#ifdef RASPBERRY_PI
    Json::Value response;
    try
    {
        PowerData data = sensorManager.getPowerData();
        
        response["status"] = "success";
        response["data"]["voltage"] = data.voltage;
        response["data"]["current"] = data.current;
        response["data"]["power"] = data.power;
        response["data"]["apparent_power"] = data.apparent_power;
        response["data"]["reactive_power"] = data.reactive_power;
        response["data"]["power_factor"] = data.power_factor;
        response["data"]["frequency"] = data.frequency;
        response["data"]["energy"] = data.energy;
        response["data"]["timestamp"] = static_cast<Json::Int64>(data.timestamp);
        response["data"]["temperature"] = sensorManager.getCpuTemperature();
        
        auto stats = sensorManager.getStatistics(300);
        for (const auto& pair : stats)
            response["stats"][pair.first] = pair.second;
    }
    catch (const std::exception& e)
    {
        response["status"] = "error";
        response["message"] = std::string(e.what());
    }
    
    Json::StreamWriterBuilder builder;
    return Json::writeString(builder, response);
#else
    return "";
#endif
}

std::string HTTPServer::handleEnergyRequest()
{
#ifdef RASPBERRY_PI
    Json::Value response;
    try
    {
        EnergyRecord latest = statistics.getLatestRecord();
        
        response["status"] = "success";
        response["data"]["energy"] = latest.energy;
        response["data"]["cost"] = latest.cost;
        response["data"]["timestamp"] = static_cast<Json::Int64>(latest.timestamp);
        
        auto today = statistics.getTodayStats();
        auto week = statistics.getWeekStats();
        auto month = statistics.getMonthStats();
        
        Json::Value todayJson;
        for (const auto& pair : today)
            todayJson[pair.first] = pair.second;
        response["stats"]["today"] = todayJson;
        
        Json::Value weekJson;
        for (const auto& pair : week)
            weekJson[pair.first] = pair.second;
        response["stats"]["week"] = weekJson;
        
        Json::Value monthJson;
        for (const auto& pair : month)
            monthJson[pair.first] = pair.second;
        response["stats"]["month"] = monthJson;
        
        float totalEnergy = today["energy_total"];
        response["environment"]["co2_kg"] = statistics.calculateCO2Emissions(totalEnergy);
        
    }
    catch (const std::exception& e)
    {
        response["status"] = "error";
        response["message"] = std::string(e.what());
    }
    
    Json::StreamWriterBuilder builder;
    return Json::writeString(builder, response);
#else
    return "";
#endif
}

std::string HTTPServer::handleStatsRequest(const std::string& period)
{
#ifdef RASPBERRY_PI
    Json::Value response; 
    try
    {
        response["status"] = "success";
        response["period"] = period;
        
        if (period == "today")
        {
            auto stats = statistics.getTodayStats();
            for (const auto& pair : stats)
                response["data"][pair.first] = pair.second;
        }
        else if (period == "yesterday")
        {
            auto stats = statistics.getYesterdayStats();
            for (const auto& pair : stats)
                response["data"][pair.first] = pair.second;
        }
        else if (period == "week")
        {
            auto stats = statistics.getWeekStats();
            for (const auto& pair : stats)
                response["data"][pair.first] = pair.second;
        }
        else if (period == "month")
        {
            auto stats = statistics.getMonthStats();
            for (const auto& pair : stats)
                response["data"][pair.first] = pair.second;
        }
        else
        {
            response["status"] = "error";
            response["message"] = "Invalid period. Use: today, yesterday, week, month";
        }
        
    }
    catch (const std::exception& e)
    {
        response["status"] = "error";
        response["message"] = std::string(e.what());
    }
    
    Json::StreamWriterBuilder builder;
    return Json::writeString(builder, response);
#else
    return "";
#endif
}