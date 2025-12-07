#include "../includes/RelayController.h"
#include "../includes/Logger.h"

bool RelayController::Initialize(int pin, bool simulation, bool activeLowMode)
{
    m_RelayPin = pin;
    m_ActiveLow = activeLowMode;
    
    LOG_INFO("Initializing relay controller on pin " + std::to_string(pin) + ", activeLow: " + std::string(m_ActiveLow ? "true" : "false"));
    
    if (!m_Gpio.Initialize(pin, simulation))
    {
        LOG_ERROR("Failed to initialize GPIO for relay");
        return false;
    }
    
    if (!m_Gpio.SetPinMode(pin, Pins::Output))
    {
        LOG_ERROR("Failed to set pin mode to OUTPUT");
        return false;
    }
    
    bool success = TurnOff();
    LOG_INFO(success ? "Relay controller initialized successfully" : "Failed to set initial relay state");
    return success;
}

void RelayController::Shutdown()
{
    LOG_INFO("Shutting down relay controller");
    m_Gpio.Cleanup();
    m_CurrentState = RelayState::UNKNOWN;
}

bool RelayController::SetRelayStateInternal(RelayState state)
{
    std::lock_guard<std::mutex> lock(m_StateMutex);
    
    bool gpioState;
    if (m_ActiveLow)
        gpioState = (state == RelayState::OFF) ? Pins::High : Pins::Low;
    else
        gpioState = (state == RelayState::ON) ? Pins::High : Pins::Low;
    
    if (m_Gpio.DigitalWrite(m_RelayPin, gpioState))
    {
        m_CurrentState = state;
        LOG_INFO("Relay set to: " + StateToString(state) + " (GPIO: " + std::string(gpioState ? "HIGH" : "LOW") + ")");
        return true;
    }
    
    LOG_ERROR("Failed to set relay state");
    return false;
}

std::string RelayController::StateToString(RelayState state)
{
    switch (state)
    {
        case RelayState::OFF:
            return "OFF";
        case RelayState::ON:
            return "ON";
        case RelayState::UNKNOWN:
            return "UNKNOWN";
        default:
            return "INVALID";
    }
}

bool RelayController::TurnOn()
{
    return SetRelayStateInternal(RelayState::ON);
}

bool RelayController::TurnOff()
{
    return SetRelayStateInternal(RelayState::OFF);
}

bool RelayController::Toggle()
{
    if (m_CurrentState == RelayState::ON)
        return TurnOff();
    else if (m_CurrentState == RelayState::OFF)
        return TurnOn();
    else
    {
        LOG_WARNING("Cannot toggle - relay state unknown");
        return false;
    }
}

RelayState RelayController::GetState() const
{
    return m_CurrentState;
}

std::string RelayController::GetStateString()
{
    return StateToString(m_CurrentState);
}

bool RelayController::IsOn() const
{
    return m_CurrentState == RelayState::ON;
}

bool RelayController::IsOff() const
{
    return m_CurrentState == RelayState::OFF;
}

void RelayController::SetActiveLow(bool activeLowMode)
{
    m_ActiveLow = activeLowMode;
    LOG_INFO("Set relay activeLow mode to: " + std::string(m_ActiveLow ? "true" : "false"));
}