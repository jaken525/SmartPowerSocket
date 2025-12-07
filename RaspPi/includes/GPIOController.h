#pragma once

#include <string>

namespace Pins
{
    static constexpr int Input = 0;
    static constexpr int Output = 1;
    static constexpr int Low = 0;
    static constexpr int High = 1;
}

class GPIOController
{
private:
    bool InitializeRealGPIO();
    bool SetPinModeReal(int pin, int mode);
    bool WritePinReal(int pin, int value);
    int ReadPinReal(int pin);
    
    bool InitializeSimulation();
    bool SetPinModeSim(int pin, int mode);
    bool WritePinSim(int pin, int value);
    int ReadPinSim(int pin);
    
public:
    GPIOController() {};
    ~GPIOController();
    
    bool Initialize(int pin, bool simulation = false);
    void Cleanup();
    
    bool SetPinMode(int pin, int mode);
    bool WritePin(int pin, int value);
    int ReadPin(int pin);
    
    bool DigitalWrite(int pin, bool state);
    bool DigitalRead(int pin);
    
    bool SetPinHigh(int pin);
    bool SetPinLow(int pin);
    bool TogglePin(int pin);
    
    bool GetIsSimulationMode() const { return m_IsSimulation; }
    bool GetInitialized() const { return m_IsInitialized; }

private:
    int m_PinNumber {-1};
    bool m_IsSimulation {false};
    bool m_IsInitialized {false};
};