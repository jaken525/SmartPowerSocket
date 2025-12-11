#pragma once

#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <string>
#include <vector>

struct PowerData
{
    float voltage;
    float current;
    float power;
    float apparent_power;
    float reactive_power;
    float power_factor;
    float frequency;
    float energy;
    uint64_t timestamp;
};

class PowerMonitor
{
private:
    bool initializeI2C();
    bool initializeAnalog();
    bool initializePZEM();
    
    PowerData readFromI2C();
    PowerData readFromAnalog();
    PowerData readFromPZEM();
    PowerData simulateData();
    
    void monitoringLoop();
    void updateStatistics(const PowerData& data);
    
public:
    enum SensorType {
        SENSOR_NONE = 0,
        SENSOR_I2C,
        SENSOR_ANALOG,
        SENSOR_PZEM,
        SENSOR_SIMULATION
    };
    
    PowerMonitor();
    ~PowerMonitor();
    
    bool initialize(SensorType type = SENSOR_SIMULATION, 
                   int bus = 1, 
                   int address = 0x40,
                   float calFactor = 1.0);
    
    void stop();
    
    PowerData getCurrentData();
    PowerData getLastValidData() const { return lastValidData; }
    
    float getVoltage() const;
    float getCurrent() const;
    float getPower() const;
    float getEnergy() const;
    float getPowerFactor() const;
    
    float getAveragePower(int seconds = 60);
    float getMaxPower(int seconds = 60);
    float getMinPower(int seconds = 60);
    
    void resetEnergy();
    
    bool isInitialized() const;
    bool isDataValid() const;
    
    void simulateLoad(float power);

private:
    std::atomic<bool> running {false};
    std::thread monitoringThread;
    std::mutex dataMutex;
    
    PowerData currentData;
    PowerData lastValidData;
    
    int i2cAddress {0x40};
    int i2cBus {1};
    bool simulationMode {true};
    float calibrationFactor {1.0};
    
    std::vector<float> powerHistory;
    size_t historySize {3600}; 
};