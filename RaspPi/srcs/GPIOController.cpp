#include "../includes/GPIOController.h"
#include "../includes/Logger.h"

#include <map>
#include <thread>
#include <chrono>

#ifdef RASPBERRY_PI
#include <wiringPi.h>
#endif

GPIOController::~GPIOController()
{
    Cleanup();
}

bool GPIOController::Initialize(int pin, bool simulation)
{
    m_PinNumber = pin;
    m_IsSimulation = simulation;
    
    if (m_IsSimulation)
    {
        LOG_INFO("Initializing GPIO in simulation mode, pin: " + std::to_string(pin));
        m_IsInitialized = InitializeSimulation();
    }
    else
    {
        LOG_INFO("Initializing real GPIO, pin: " + std::to_string(pin));
        m_IsInitialized = InitializeRealGPIO();
    }
    
    LOG_INFO(m_IsInitialized ? "GPIO initialized successfully" : "Failed to initialize GPIO");
    return m_IsInitialized;
}

bool GPIOController::InitializeRealGPIO()
{
#ifdef RASPBERRY_PI
    try
    {
        if (wiringPiSetup() == -1)
        {
            LOG_ERROR("Failed to initialize wiringPi");
            return false;
        }
        LOG_INFO("wiringPi initialized successfully");
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in wiringPi initialization: " + std::string(e.what()));
        return false;
    }
#else
    LOG_WARNING("Not on Raspberry Pi, using simulation mode");
    m_IsSimulation = true;
    return InitializeSimulation();
#endif
}

bool GPIOController::InitializeSimulation()
{
    LOG_INFO("Simulation mode activated - no real GPIO operations");
    return true;
}

void GPIOController::Cleanup() {
    if (m_IsInitialized)
    {
        m_IsInitialized = false;
        LOG_INFO("GPIO cleanup completed");
    }
}

bool GPIOController::SetPinMode(int pin, int mode)
{
    if (!m_IsInitialized)
    {
        LOG_ERROR("GPIO not initialized");
        return false;
    }
    
    return m_IsSimulation ? SetPinModeSim(pin, mode) : SetPinModeReal(pin, mode); 
}

bool GPIOController::SetPinModeReal(int pin, int mode)
{
#ifdef RASPBERRY_PI
    try
    {
        pinMode(pin, mode);
        LOG_DEBUG("Set pin " + std::to_string(pin) + " mode to " + (mode == Pins::Input ? "INPUT" : "OUTPUT"));
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to set pin mode: " + std::string(e.what()));
        return false;
    }
#else
    return false;
#endif
}

bool GPIOController::SetPinModeSim(int pin, int mode)
{
    LOG_DEBUG("[SIM] Set pin " + std::to_string(pin) + " mode to " + (mode == Pins::Input ? "INPUT" : "OUTPUT"));
    return true;
}

bool GPIOController::WritePin(int pin, int value)
{
    if (!m_IsInitialized)
    {
        LOG_ERROR("GPIO not initialized");
        return false;
    }
    
    return m_IsSimulation ? WritePinSim(pin, value) : WritePinReal(pin, value);
}

bool GPIOController::WritePinReal(int pin, int value)
{
#ifdef RASPBERRY_PI
    try
    {
        digitalWrite(pin, value);
        LOG_DEBUG("Set pin " + std::to_string(pin) + " to " + (value == Pins::High ? "HIGH" : "LOW"));
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to write pin: " + std::string(e.what()));
        return false;
    }
#else
    return false;
#endif
}

bool GPIOController::WritePinSim(int pin, int value)
{
    LOG_DEBUG("[SIM] Set pin " + std::to_string(pin) + " to " + (value == Pins::High ? "HIGH" : "LOW"));
    return true;
}

int GPIOController::ReadPin(int pin)
{
    if (!m_IsInitialized)
    {
        LOG_ERROR("GPIO not initialized");
        return -1;
    }
    
    return m_IsSimulation ? ReadPinSim(pin) : ReadPinReal(pin);
}

int GPIOController::ReadPinReal(int pin)
{
#ifdef RASPBERRY_PI
    try
    {
        int value = digitalRead(pin);
        LOG_DEBUG("Read pin " + std::to_string(pin) + " = " + std::to_string(value));
        return value;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to read pin: " + std::string(e.what()));
        return -1;
    }
#else
    return -1;
#endif
}

int GPIOController::ReadPinSim(int pin)
{
    LOG_DEBUG("[SIM] Read pin " + std::to_string(pin) + " = 0");
    return Pins::Low;
}

bool GPIOController::DigitalWrite(int pin, bool state)
{
    return WritePin(pin, state ? Pins::High : Pins::Low);
}

bool GPIOController::DigitalRead(int pin)
{
    int value = ReadPin(pin);
    return (value == Pins::High);
}

bool GPIOController::SetPinHigh(int pin)
{
    return WritePin(pin, Pins::High);
}

bool GPIOController::SetPinLow(int pin)
{
    return WritePin(pin, Pins::Low);
}

bool GPIOController::TogglePin(int pin)
{
    if (!m_IsInitialized)
        return false;

    int current = ReadPin(pin);
    if (current == -1)
        return false;
    
    return WritePin(pin, current == Pins::High ? Pins::Low : Pins::High);
}