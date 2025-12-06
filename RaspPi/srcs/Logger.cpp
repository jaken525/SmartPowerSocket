#include "..\includes\Logger.h"

Logger* Logger::m_Instance = nullptr;

Logger& Logger::GetInstance()
{
    static std::once_flag initFlag;
    std::call_once(initFlag, []() {
        m_Instance = new Logger();
    });
    return *m_Instance;
}

Logger::~Logger()
{
    if (m_LogFile.is_open())
        m_LogFile.close();
}

std::string Logger::GetCurrentTime()
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

std::string Logger::LevelToString(LogLevel level)
{
    switch(level)
    {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARNING:
            return "WARNING";
        case LogLevel::ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

void Logger::SetLogLevel(LogLevel level)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_CurrentLevel = level;
}

void Logger::EnableConsoleOutput(bool enable)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_WriteToConsole = enable;
}

void Logger::EnableFileOutput(bool enable, const std::string& filename)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    if (enable && !filename.empty())
    {
        if (m_LogFile.is_open())
            m_LogFile.close();

        m_LogFile.open(filename, std::ios::app);
        m_WriteToFile = m_LogFile.is_open();
        if (!m_WriteToFile)
            std::cerr << "Failed to open log file: " << filename << std::endl;
    }
    else
    {
        m_WriteToFile = false;
        if (m_LogFile.is_open())
            m_LogFile.close();
    }
}

void Logger::Log(LogLevel level, const std::string& message, const std::string& function, int line)
{
    if (level < m_CurrentLevel)
        return;
    
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    std::stringstream logEntry;
    logEntry << "[" << GetCurrentTime() << "] "
             << "[" << LevelToString(level) << "] ";
    
    if (!function.empty())
        logEntry << "[" << function << ":" << line << "] ";
    
    logEntry << message;
    
    std::string fullMessage = logEntry.str();
    if (m_WriteToConsole)
        std::cout << fullMessage << std::endl;
    
    if (m_WriteToFile && m_LogFile.is_open())
    {
        m_LogFile << fullMessage << std::endl;
        m_LogFile.flush();
    }
}

void Logger::Debug(const std::string& message, const std::string& function, int line)
{
    Log(LogLevel::DEBUG, message, function, line);
}

void Logger::Info(const std::string& message, const std::string& function, int line)
{
    Log(LogLevel::INFO, message, function, line);
}

void Logger::Warning(const std::string& message, const std::string& function, int line)
{
    Log(LogLevel::WARNING, message, function, line);
}

void Logger::Error(const std::string& message, const std::string& function, int line)
{
    Log(LogLevel::ERROR, message, function, line);
}