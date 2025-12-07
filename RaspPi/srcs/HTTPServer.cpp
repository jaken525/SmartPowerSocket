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