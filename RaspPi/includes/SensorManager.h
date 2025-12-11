#pragma once

#include "PowerMonitor.h"

#include <functional>
#include <vector>
#include <map>
#include <string>

struct SensorConfig
{
    PowerMonitor::SensorType type;
    int bus;
    int address;
    float calibration;
    std::string name;
    bool enabled;
};

class SensorManager
{
private:    
    void updateCpuTemperature();
    void checkThresholds(const PowerData& data);
    
public:
    SensorManager() {};
    
    bool initialize(const SensorConfig& config);
    void shutdown();
    
    void setPowerThresholds(float warning, float critical);
    void setTemperatureThreshold(float warning);
    
    PowerData getPowerData();
    float getCpuTemperature();
    
    std::map<std::string, float> getStatistics(int periodSeconds = 300);
    
    void resetEnergyCounter();
    void calibrate(float referenceValue);
    
    void setPowerThresholdCallback(std::function<void(float, float)> callback);
    void setTemperatureCallback(std::function<void(float)> callback);
    
    bool isPowerSensorActive() const;
    std::string getSensorStatus() const;
    
    void simulatePowerSpike(float power, int durationMs);

private:
    PowerMonitor powerMonitor;
    SensorConfig currentConfig;
    
    float cpuTemperature {0.0f};
    
    float powerWarningThreshold {2000.0f};
    float powerCriticalThreshold {3000.0f};
    float temperatureWarningThreshold {70.0f};
    
    std::function<void(float, float)> powerThresholdCallback;
    std::function<void(float)> temperatureCallback;
};