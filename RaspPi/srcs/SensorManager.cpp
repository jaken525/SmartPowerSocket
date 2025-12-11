#include "../includes/SensorManager.h"
#include "../includes/Logger.h"
#include <fstream>
#include <sstream>

bool SensorManager::initialize(const SensorConfig& config)
{
    currentConfig = config;
    
    if (!config.enabled)
    {
        LOG_INFO("Sensor monitoring disabled");
        return true;
    }
    
    LOG_INFO("Initializing sensor: " + config.name + 
             " (Type: " + std::to_string(static_cast<int>(config.type)) + 
             ", Bus: " + std::to_string(config.bus) + 
             ", Addr: 0x" + std::to_string(config.address) + ")");
    
    bool success = powerMonitor.initialize(config.type, config.bus, config.address, config.calibration);
    
    if (success)
    {
        LOG_INFO("Sensor manager initialized successfully");
        updateCpuTemperature();
    }
    
    return success;
}

void SensorManager::shutdown()
{
    powerMonitor.stop();
    LOG_INFO("Sensor manager shut down");
}

void SensorManager::updateCpuTemperature()
{
    std::ifstream tempFile("/sys/class/thermal/thermal_zone0/temp");
    if (tempFile.is_open())
    {
        std::string line;
        if (std::getline(tempFile, line))
        {
            try
            {
                cpuTemperature = std::stof(line) / 1000.0f;
            }
            catch (...)
            {
                cpuTemperature = 0.0f;
            }
        }
        tempFile.close();
    }
}

void SensorManager::checkThresholds(const PowerData& data)
{
    if (data.power >= powerCriticalThreshold && powerThresholdCallback)
        powerThresholdCallback(data.power, powerCriticalThreshold);
    else if (data.power >= powerWarningThreshold && powerThresholdCallback)
        powerThresholdCallback(data.power, powerWarningThreshold);

    if (cpuTemperature >= temperatureWarningThreshold && temperatureCallback)
        temperatureCallback(cpuTemperature);
}

void SensorManager::setPowerThresholds(float warning, float critical)
{
    powerWarningThreshold = warning;
    powerCriticalThreshold = critical;
    LOG_INFO("Power thresholds set: Warning=" + 
             std::to_string(warning) + "W, Critical=" + 
             std::to_string(critical) + "W");
}

void SensorManager::setTemperatureThreshold(float warning)
{
    temperatureWarningThreshold = warning;
    LOG_INFO("Temperature warning threshold set: " + std::to_string(warning) + "°C");
}

PowerData SensorManager::getPowerData()
{
    PowerData data = powerMonitor.getCurrentData();

    static auto lastTempUpdate = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    
    if (std::chrono::duration_cast<std::chrono::seconds>(now - lastTempUpdate).count() >= 10)
    {
        updateCpuTemperature();
        lastTempUpdate = now;
    }
    
    checkThresholds(data);
    return data;
}

float SensorManager::getCpuTemperature()
{
    updateCpuTemperature();
    return cpuTemperature;
}

std::map<std::string, float> SensorManager::getStatistics(int periodSeconds)
{
    std::map<std::string, float> stats;
    
    PowerData current = powerMonitor.getCurrentData();
    
    stats["voltage"] = current.voltage;
    stats["current"] = current.current;
    stats["power"] = current.power;
    stats["power_apparent"] = current.apparent_power;
    stats["power_reactive"] = current.reactive_power;
    stats["power_factor"] = current.power_factor;
    stats["frequency"] = current.frequency;
    stats["energy"] = current.energy;
    stats["temperature"] = cpuTemperature;
    stats["power_avg"] = powerMonitor.getAveragePower(periodSeconds);
    stats["power_max"] = powerMonitor.getMaxPower(periodSeconds);
    stats["power_min"] = powerMonitor.getMinPower(periodSeconds);
    stats["load_percentage"] = current.voltage > 0 ? (current.power / powerCriticalThreshold) * 100.0f : 0.0f;

    return stats;
}

void SensorManager::resetEnergyCounter()
{
    powerMonitor.resetEnergy();
}

void SensorManager::calibrate(float referenceValue)
{
    LOG_INFO("Calibration requested with reference: " + std::to_string(referenceValue));
}

void SensorManager::setPowerThresholdCallback(std::function<void(float, float)> callback)
{
    powerThresholdCallback = callback;
}

void SensorManager::setTemperatureCallback(std::function<void(float)> callback)
{
    temperatureCallback = callback;
}

bool SensorManager::isPowerSensorActive() const
{
    return currentConfig.enabled && powerMonitor.isDataValid();
}

std::string SensorManager::getSensorStatus() const
{
    if (!currentConfig.enabled)
        return "disabled";
    
    if (!powerMonitor.isDataValid())
        return "no_data";

    PowerData data = powerMonitor.getLastValidData();
    if (data.voltage == 0 && data.current == 0)
        return "idle";
    
    return "active";
}

void SensorManager::simulatePowerSpike(float power, int durationMs)
{
    LOG_INFO("Simulating power spike: " + std::to_string(power) + "W for " + std::to_string(durationMs) + "ms");
    powerMonitor.simulateLoad(power);
}