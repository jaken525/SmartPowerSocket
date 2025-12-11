#include "../includes/PowerMonitor.h"
#include "../includes/Logger.h"
#include "../includes/ConfigManager.h"
#include <cmath>
#include <algorithm>
#include <random>

#ifdef HAVE_I2C
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#endif

PowerMonitor::PowerMonitor()
{
    currentData = {0, 0, 0, 0, 0, 1.0, 50.0, 0, 0};
    lastValidData = currentData;
    powerHistory.resize(historySize, 0.0f);
}

PowerMonitor::~PowerMonitor()
{
    stop();
}

bool PowerMonitor::initialize(SensorType type, int bus, int address, float calFactor)
{
    if (running)
        stop();

    i2cBus = bus;
    i2cAddress = address;
    calibrationFactor = calFactor;
    simulationMode = (type == SENSOR_SIMULATION);
    
    bool initialized = false;
    
    switch (type)
    {
        case SENSOR_I2C:
            initialized = initializeI2C();
            break;
        case SENSOR_ANALOG:
            initialized = initializeAnalog();
            break;
        case SENSOR_PZEM:
            initialized = initializePZEM();
            break;
        case SENSOR_SIMULATION:
            initialized = true;
            LOG_INFO("Power monitor running in simulation mode");
            break;
        case SENSOR_NONE:
            LOG_INFO("Power monitoring disabled");
            return true;
    }
    
    if (initialized)
    {
        running = true;
        monitoringThread = std::thread(&PowerMonitor::monitoringLoop, this);
        LOG_INFO("Power monitor initialized successfully");
    }
    else
    {
        LOG_ERROR("Failed to initialize power monitor");
        simulationMode = true;
        running = true;
        monitoringThread = std::thread(&PowerMonitor::monitoringLoop, this);
        LOG_WARNING("Power monitor running in simulation mode (fallback)");
    }
    
    return true;
}

bool PowerMonitor::initializeI2C()
{
#ifdef HAVE_I2C
    LOG_INFO("Initializing I2C power sensor on bus " + 
             std::to_string(i2cBus) + ", address 0x" + 
             std::to_string(i2cAddress));
    return true;
#else
    LOG_WARNING("I2C support not compiled in");
    return false;
#endif
}

bool PowerMonitor::initializeAnalog()
{
    LOG_INFO("Initializing analog power sensor");
    return false; // Пока не реализовано
}

bool PowerMonitor::initializePZEM()
{
    LOG_INFO("Initializing PZEM-004T power sensor");
    return false; // Пока не реализовано
}

void PowerMonitor::monitoringLoop()
{
    LOG_INFO("Power monitoring thread started");
    
    auto lastStatUpdate = std::chrono::steady_clock::now();
    auto lastLogUpdate = std::chrono::steady_clock::now();
    
    while (running)
    {
        PowerData newData;
        
        if (simulationMode)
            newData = simulateData();
        else
        {
            // реальное чтение с датчика
            // newData = readFromI2C(); или readFromAnalog() и т.д.
            newData = simulateData(); // временно использую симуляцию
        }
        
        newData.current *= calibrationFactor;
        newData.power *= calibrationFactor;

        {
            std::lock_guard<std::mutex> lock(dataMutex);
            currentData = newData;
            currentData.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            
            if (newData.voltage > 0 && newData.current >= 0)
                lastValidData = newData;
        }
        
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastStatUpdate).count() >= 1)
        {
            updateStatistics(newData);
            lastStatUpdate = now;
        }
        
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastLogUpdate).count() >= 30)
        {
            LOG_DEBUG("Power: " + std::to_string(newData.power) + 
                     "W, Current: " + std::to_string(newData.current) + 
                     "A, Voltage: " + std::to_string(newData.voltage) + "V");
            lastLogUpdate = now;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    LOG_INFO("Power monitoring thread stopped");
}

PowerData PowerMonitor::simulateData()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> voltDist(215.0, 230.0);
    static std::uniform_real_distribution<> freqDist(49.8, 50.2);
    static std::uniform_real_distribution<> pfDist(0.85, 0.99);
    
    static float simulatedLoad = 100.0f;
    static float energyAccumulator = 0.0f;
    static auto lastUpdate = std::chrono::steady_clock::now();
    
    auto now = std::chrono::steady_clock::now();
    float deltaHours = std::chrono::duration<float>(now - lastUpdate).count() / 3600.0f;
    lastUpdate = now;

    energyAccumulator += simulatedLoad * deltaHours / 1000.0f;
    
    PowerData data;
    data.voltage = voltDist(gen);
    data.current = simulatedLoad / data.voltage;
    data.power = simulatedLoad;
    data.apparent_power = data.voltage * data.current;
    data.reactive_power = std::sqrt(data.apparent_power * data.apparent_power - data.power * data.power);
    data.power_factor = pfDist(gen);
    data.frequency = freqDist(gen);
    data.energy = energyAccumulator;
    
    return data;
}

void PowerMonitor::updateStatistics(const PowerData& data)
{
    std::rotate(powerHistory.begin(), powerHistory.begin() + 1, powerHistory.end());
    
    powerHistory[historySize - 1] = data.power;
}

PowerData PowerMonitor::getCurrentData()
{
    std::lock_guard<std::mutex> lock(dataMutex);
    return currentData;
}

float PowerMonitor::getVoltage() const
{
    return lastValidData.voltage;
}

float PowerMonitor::getCurrent() const
{
    return lastValidData.current;
}

float PowerMonitor::getPower() const
{
    return lastValidData.power;
}

float PowerMonitor::getEnergy() const
{
    return lastValidData.energy;
}

float PowerMonitor::getPowerFactor() const
{
    return lastValidData.power_factor;
}

float PowerMonitor::getAveragePower(int seconds)
{
    if (seconds <= 0 || seconds > static_cast<int>(historySize))
        seconds = 60;

    size_t samples = std::min(static_cast<size_t>(seconds), historySize);
    float sum = 0.0f;
    size_t count = 0;
    
    for (size_t i = historySize - samples; i < historySize; i++)
        if (powerHistory[i] > 0)
        {
            sum += powerHistory[i];
            count++;
        }
    
    return count > 0 ? sum / count : 0.0f;
}

float PowerMonitor::getMaxPower(int seconds)
{
    if (seconds <= 0 || seconds > static_cast<int>(historySize))
        seconds = 60;

    size_t samples = std::min(static_cast<size_t>(seconds), historySize);
    float maxPower = 0.0f;
    
    for (size_t i = historySize - samples; i < historySize; i++)
        if (powerHistory[i] > maxPower)
            maxPower = powerHistory[i];

    return maxPower;
}

float PowerMonitor::getMinPower(int seconds)
{
    if (seconds <= 0 || seconds > static_cast<int>(historySize))
        seconds = 60;

    size_t samples = std::min(static_cast<size_t>(seconds), historySize);
    float minPower = std::numeric_limits<float>::max();
    
    for (size_t i = historySize - samples; i < historySize; i++)
        if (powerHistory[i] > 0 && powerHistory[i] < minPower)
            minPower = powerHistory[i];

    return (minPower < std::numeric_limits<float>::max()) ? minPower : 0.0f;
}

void PowerMonitor::resetEnergy()
{
    std::lock_guard<std::mutex> lock(dataMutex);
    currentData.energy = 0;
    lastValidData.energy = 0;
    LOG_INFO("Energy counter reset");
}

bool PowerMonitor::isInitialized() const
{
    return running;
}

bool PowerMonitor::isDataValid() const
{
    return lastValidData.voltage > 0;
}

void PowerMonitor::stop()
{
    if (running)
    {
        running = false;
        if (monitoringThread.joinable())
            monitoringThread.join();
    }
}

void PowerMonitor::simulateLoad(float power)
{
    LOG_INFO("Setting simulated load to " + std::to_string(power) + "W");
    // в реальной реализации здесь будет изменение параметров симуляции
}