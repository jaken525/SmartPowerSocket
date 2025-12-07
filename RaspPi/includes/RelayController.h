#pragma once

#include "GPIOController.h"

#include <string>
#include <mutex>

enum class RelayState
{
    OFF,
    ON,
    UNKNOWN
};

class RelayController
{
private:
    bool SetRelayStateInternal(RelayState state);
    std::string StateToString(RelayState state);
    
public:
    RelayController() {};
    
    bool Initialize(int pin, bool simulation = false, bool activeLow = false);
    void Shutdown();
    
    bool TurnOn();
    bool TurnOff();
    bool Toggle();
    
    RelayState GetState() const;
    std::string GetStateString();
    
    bool IsOn() const;
    bool IsOff() const;
    
    void SetActiveLow(bool activeLowMode);

private:
    GPIOController m_Gpio;
    int m_RelayPin {-1};
    RelayState m_CurrentState {RelayState::UNKNOWN};
    std::mutex m_StateMutex;
    bool m_ActiveLow {false};
};