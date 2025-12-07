#pragma once

#include <string>
#include <map>
#include <mutex>

class ConfigManager
{
private:    
    ConfigManager();
    bool ParseConfigFile();
    
public:
    static ConfigManager& GetInstance();
    
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    bool LoadConfig(const std::string& path);
    bool SaveConfig();
    
    std::string GetString(const std::string& key, const std::string& defaultValue = "");
    int GetInt(const std::string& key, int defaultValue = 0);
    bool GetBool(const std::string& key, bool defaultValue = false);
    
    void SetString(const std::string& key, const std::string& value);
    void SetInt(const std::string& key, int value);
    void SetBool(const std::string& key, bool value);
    
    void PrintConfig() const;
    
    int GetServerPort() const;
    std::string GetServerAddress() const;
    int GetGPIOPin() const;
    bool GetSimulationMode() const;
    int GetLogLevel() const;

private:
    static ConfigManager* instance;
    std::string configPath;
    std::map<std::string, std::string> configData;
    std::mutex configMutex;
};