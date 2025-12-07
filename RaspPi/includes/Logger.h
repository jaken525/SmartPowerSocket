#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

enum class LogLevel
{
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3
};

class Logger
{
private:    
    Logger() {};
    std::string GetCurrentTime();
    std::string LevelToString(LogLevel level);
    
public:
    static Logger& GetInstance();
    ~Logger();
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    void SetLogLevel(LogLevel level);
    void EnableConsoleOutput(bool enable);
    void EnableFileOutput(bool enable, const std::string& filename = "");
    
    void Log(LogLevel level, const std::string& message, const std::string& function = "", int line = 0);
    void Debug(const std::string& message, const std::string& function = "", int line = 0);
    void Info(const std::string& message, const std::string& function = "", int line = 0);
    void Warning(const std::string& message, const std::string& function = "", int line = 0);
    void Error(const std::string& message, const std::string& function = "", int line = 0);

private:
    static Logger* m_Instance;
    std::ofstream m_LogFile;
    std::mutex m_Mutex;
    LogLevel m_CurrentLevel {LogLevel::INFO};
    bool m_WriteToConsole {true};
    bool m_WriteToFile {false};
};

#define LOG_DEBUG(msg) Logger::GetInstance().Debug(msg, __FUNCTION__, __LINE__)
#define LOG_INFO(msg) Logger::GetInstance().Info(msg, __FUNCTION__, __LINE__)
#define LOG_WARNING(msg) Logger::GetInstance().Warning(msg, __FUNCTION__, __LINE__)
#define LOG_ERROR(msg) Logger::GetInstance().Error(msg, __FUNCTION__, __LINE__)