#include <iostream>
#include <csignal>
#include <atomic>
#include "../includes/HTTPServer.h"
#include "../includes/RelayController.h"
#include "../includes/ConfigManager.h"
#include "../includes/Logger.h"

std::atomic<bool> running{true};

void SignalHandler(int signal)
{
    LOG_INFO("Received signal: " + std::to_string(signal));
    running = false;
}

void SetupSignalHandlers()
{
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);
}

int main(int argc, char** argv)
{
    SetupSignalHandlers();

    ConfigManager& config = ConfigManager::GetInstance();
    
    std::string configPath = "config/config.json";
    if (argc > 1)
        configPath = argv[1];
    
    if (!config.LoadConfig(configPath))
        std::cerr << "Warning: Using default configuration\n";
    
    config.PrintConfig();

    Logger& logger = Logger::GetInstance();
    logger.SetLogLevel(static_cast<LogLevel>(config.GetLogLevel()));
    logger.EnableConsoleOutput(config.GetBool("log.console", true));
    
    if (config.GetBool("log.file", false))
    {
        std::string logFile = config.GetString("log.file", "logs/smart_plug.log");
        logger.EnableFileOutput(true, logFile);
    }
    
    LOG_INFO("Starting Smart Plug Server...");
    
    RelayController relay;
    int gpioPin = config.GetGPIOPin();
    bool simulationMode = config.GetSimulationMode();
    
    if (!relay.Initialize(gpioPin, simulationMode))
    {
        LOG_ERROR("Failed to initialize relay controller");
        return 1;
    }
    
    LOG_INFO("Relay controller initialized on GPIO pin: " + std::to_string(gpioPin));
    
    std::string defaultState = config.GetString("relay.default_state", "off");
    if (defaultState == "on")
        relay.TurnOn();
    else
        relay.TurnOff();
    
    SensorManager sensorManager;
    SensorConfig sensorConfig;
    
    sensorConfig.type = static_cast<PowerMonitor::SensorType>(config.GetInt("sensor.type", 0));
    sensorConfig.bus = config.GetInt("sensor.bus", 1);
    sensorConfig.address = config.GetInt("sensor.address", 0x40);
    sensorConfig.calibration = config.GetFloat("sensor.calibration", 1.0);
    sensorConfig.name = config.GetString("sensor.name", "default");
    sensorConfig.enabled = config.GetBool("sensor.enabled", false);
    
    if (!sensorManager.initialize(sensorConfig))
        LOG_ERROR("Failed to initialize sensor manager");
    else
    {
        float warningThreshold = config.GetFloat("sensor.warning_threshold", 2000.0f);
        float criticalThreshold = config.GetFloat("sensor.critical_threshold", 3000.0f);
        sensorManager.setPowerThresholds(warningThreshold, criticalThreshold);
        
        sensorManager.setPowerThresholdCallback(
            [](float power, float threshold) {
                LOG_WARNING("Power threshold exceeded: " + 
                           std::to_string(power) + "W > " + 
                           std::to_string(threshold) + "W");
            });
    }
    
    Statistics statistics;
    float peakTariff = config.GetFloat("tariff.peak", 5.0f);
    float offpeakTariff = config.GetFloat("tariff.offpeak", 2.0f);
    statistics.setTariffs(peakTariff, offpeakTariff);

    HTTPServer server(relay, sensorManager, statistics);

        int port = config.GetServerPort();
    std::string address = config.GetServerAddress();
    
    if (!server.Start(port, address))
    {
        LOG_ERROR("Failed to start HTTP server");
        relay.Shutdown();
        return 1;
    }
    
    LOG_INFO("Server is running. Press Ctrl+C to stop.");

    while (running)
    {
        static auto lastStatUpdate = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastStatUpdate).count() >= 60)
        {
            if (sensorManager.isPowerSensorActive())
            {
                PowerData data = sensorManager.getPowerData();
                statistics.addPowerReading(data.power, 60);
            }
            lastStatUpdate = now;
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    LOG_INFO("Shutting down server...");
    server.Stop();
    relay.Shutdown();
    sensorManager.shutdown();
    
    LOG_INFO("Server stopped successfully");
    
    return 0;
}