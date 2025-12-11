#include "../includes/ConfigManager.h"
#include "../includes/Logger.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

ConfigManager* ConfigManager::instance = nullptr;

ConfigManager::ConfigManager() : configPath("config/config.cfg")
{
    configData["server.port"] = "5000";
    configData["server.address"] = "0.0.0.0";
    configData["gpio.pin"] = "17";
    configData["gpio.simulation"] = "false";
    configData["log.level"] = "1";
    configData["log.file"] = "logs/smart_plug.log";
    configData["log.console"] = "true";
    configData["relay.default_state"] = "off";
    configData["security.api_key"] = "";
    configData["security.enable_auth"] = "false";
}

ConfigManager& ConfigManager::GetInstance()
{
    static std::once_flag initFlag;
    std::call_once(initFlag, []() {
        instance = new ConfigManager();
    });
    return *instance;
}

bool ConfigManager::LoadConfig(const std::string& path)
{
    std::lock_guard<std::mutex> lock(configMutex);
    
    if (!path.empty())
        configPath = path;

    std::ifstream file(configPath);
    if (!file.is_open()) 
    {
        LOG_WARNING("Config file not found: " + configPath + ", using defaults");
        return false;
    }
    
    std::string line;
    while (std::getline(file, line))
    {
        line.erase(std::remove_if(line.begin(), line.end(), ::isspace), 
                  line.end());
    
        if (line.empty() || line[0] == '#')
            continue;
        
        size_t delimiterPos = line.find('=');
        if (delimiterPos == std::string::npos)
            continue;
        
        std::string key = line.substr(0, delimiterPos);
        std::string value = line.substr(delimiterPos + 1);
    
        if (!value.empty() && value.front() == '"' && value.back() == '"')
            value = value.substr(1, value.length() - 2);
        
        configData[key] = value;
    }
    
    file.close();
    LOG_INFO("Config loaded from: " + configPath);
    return true;
}

bool ConfigManager::SaveConfig()
{
    std::lock_guard<std::mutex> lock(configMutex);
    
    std::ofstream file(configPath);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to save config: " + configPath);
        return false;
    }
    file << "server.port=" << configData["server.port"] << "\n";
    file << "server.address=" << configData["server.address"] << "\n\n";
    
    file << "gpio.pin=" << configData["gpio.pin"] << "\n";
    file << "gpio.simulation=" << configData["gpio.simulation"] << "\n\n";
    
    file << "log.level=" << configData["log.level"] << "\n";
    file << "log.file=\"" << configData["log.file"] << "\"\n";
    file << "log.console=" << configData["log.console"] << "\n\n";
    
    file << "relay.default_state=" << configData["relay.default_state"] << "\n\n";
    
    file << "security.api_key=\"" << configData["security.api_key"] << "\"\n";
    file << "security.enable_auth=" << configData["security.enable_auth"] << "\n";
    
    file.close();
    LOG_INFO("Config saved to: " + configPath);
    return true;
}

std::string ConfigManager::GetString(const std::string& key, const std::string& defaultValue)
{
    std::lock_guard<std::mutex> lock(configMutex);
    auto it = configData.find(key);
    if (it != configData.end())
        return it->second;

    return defaultValue;
}

int ConfigManager::GetInt(const std::string& key, int defaultValue)
{
    std::string value = GetString(key, "");
    if (value.empty())
        return defaultValue;

    try
    {
        return std::stoi(value);
    }
    catch (...)
    {
        return defaultValue;
    }
}

float ConfigManager::GetFloat(const std::string& key, int defaultValue)
{
    std::string value = GetString(key, "");
    if (value.empty())
        return defaultValue;

    try
    {
        return std::stof(value);
    }
    catch (...)
    {
        return defaultValue;
    }
}

bool ConfigManager::GetBool(const std::string& key, bool defaultValue)
{
    std::string value = GetString(key, "");
    if (value.empty())
        return defaultValue;

    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    return (value == "true" || value == "1" || value == "yes");
}

void ConfigManager::SetString(const std::string& key, const std::string& value)
{
    std::lock_guard<std::mutex> lock(configMutex);
    configData[key] = value;
}

void ConfigManager::SetInt(const std::string& key, int value)
{
    SetString(key, std::to_string(value));
}

void ConfigManager::SetBool(const std::string& key, bool value)
{
    SetString(key, value ? "true" : "false");
}

void ConfigManager::PrintConfig() const
{
    LOG_INFO("Current configuration:");
    for (const auto& pair : configData)
        LOG_INFO("  " + pair.first + " = " + pair.second);
}

int ConfigManager::GetServerPort() const
{
    return std::stoi(configData.at("server.port"));
}

std::string ConfigManager::GetServerAddress() const
{
    return configData.at("server.address");
}

int ConfigManager::GetGPIOPin() const
{
    return std::stoi(configData.at("gpio.pin"));
}

bool ConfigManager::GetSimulationMode() const 
{
    std::string value = configData.at("gpio.simulation");
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    return (value == "true");
}

int ConfigManager::GetLogLevel() const
{
    return std::stoi(configData.at("log.level"));
}