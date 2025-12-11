#pragma once

#ifdef RASPBERRY_PI
#include <microhttpd.h>
#endif

#include <string>
#include <map>
#include <functional>

#include "RelayController.h"
#include "SensorManager.h"
#include "Statistics.h"

class HTTPServer
{
private:
    static int HandleRequest(void* cls, struct MHD_Connection* connection,
                           const char* url, const char* method,
                           const char* version, const char* upload_data,
                           size_t* upload_data_size, void** con_cls);
    
    int ProcessRequest(struct MHD_Connection* connection,
                      const std::string& url, const std::string& method);
    
    std::string CreateJSONResponse(const std::string& status,
                                  const std::string& message,
                                  const std::string& relayState = "");
    
    bool CheckAuthentication(struct MHD_Connection* connection);
    std::string GetClientIP(struct MHD_Connection* connection);
    
    void LogRequest(const std::string& clientIP, 
                   const std::string& method, 
                   const std::string& url, 
                   int responseCode);
    std::string handlePowerRequest();
    std::string handleEnergyRequest();
    std::string handleStatsRequest(const std::string& period);
    std::string handleSensorConfigRequest();
    std::string handleCalibrationRequest(const std::string& params);
    
public:
    HTTPServer(RelayController& relayController, SensorManager& sensorMgr, Statistics& stats)
    : relay(relayController), sensorManager(sensorMgr), statistics(stats) {};
    ~HTTPServer();
    
    bool Start(int port = 5000, const std::string& address = "0.0.0.0");
    void Stop();
    
    bool IsRunning() const { return running; }
    int GetPort() const { return port; }
    std::string GetAddress() const { return address; }
    
    void AddAPIKey(const std::string& key, const std::string& clientName = "");

private:
    struct MHD_Daemon* daemon {nullptr};
    RelayController& relay;
    std::string address {"0.0.0.0"};
    int port {5000};
    bool running {false};
    
    std::map<std::string, std::string> apiKeys;

    SensorManager& sensorManager;
    Statistics& statistics;
};