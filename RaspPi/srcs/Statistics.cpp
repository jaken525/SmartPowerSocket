#include "../includes/Statistics.h"
#include "../includes/Logger.h"
#include <iomanip>
#include <sstream>
#include <fstream>
#include <ctime>

#ifdef RASPBERRY_PI
#include <json/json.h>
#endif

Statistics::Statistics() : peakHours({8, 23}) {}

void Statistics::setTariffs(float peak, float offpeak)
{
    std::lock_guard<std::mutex> lock(statsMutex);
    tariffPeak = peak;
    tariffOffpeak = offpeak;
    LOG_INFO("Tariffs set: Peak=" + std::to_string(peak) + ", Offpeak=" + std::to_string(offpeak));
}

void Statistics::setPeakHours(int start, int end)
{
    std::lock_guard<std::mutex> lock(statsMutex);
    peakHours = {start, end};
    LOG_INFO("Peak hours set: " + std::to_string(start) + ":00 - " + std::to_string(end) + ":00");
}

void Statistics::addEnergyReading(float energy)
{
    std::lock_guard<std::mutex> lock(statsMutex);
    
    EnergyRecord record;
    record.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    record.energy = energy;
    
    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);
    int currentHour = localTime->tm_hour;
    
    record.cost = calculateCost(energy, currentHour);
    
    energyHistory.push_back(record);
    
    if (energyHistory.size() > 43200)
        energyHistory.erase(energyHistory.begin());

    updateDailyStats(record);
}

void Statistics::addPowerReading(float power, int durationSeconds)
{
    if (durationSeconds <= 0 || power <= 0)
        return;
    
    float energy = (power * durationSeconds) / 3600000.0f;
    addEnergyReading(energy);
}

void Statistics::updateDailyStats(const EnergyRecord& record)
{
    std::time_t ts = static_cast<std::time_t>(record.timestamp);
    std::tm* time = std::localtime(&ts);
    
    char dateStr[11];
    std::strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", time);
    std::string date(dateStr);
    
    int hour = time->tm_hour;
    bool isPeakHour = (hour >= peakHours.first && hour < peakHours.second);
    
    if (dailyStats.find(date) == dailyStats.end())
    {
        DailyStats stats;
        stats.date = date;
        stats.energy_total = 0;
        stats.energy_peak = 0;
        stats.energy_offpeak = 0;
        stats.cost_total = 0;
        stats.usage_hours = 0;
        dailyStats[date] = stats;
    }
    
    DailyStats& stats = dailyStats[date];
    stats.energy_total += record.energy;
    
    if (isPeakHour)
        stats.energy_peak += record.energy;
    else
        stats.energy_offpeak += record.energy;

    stats.cost_total += record.cost;
    stats.usage_hours = static_cast<int>(stats.energy_total * 1000.0f / 60.0f);
}

float Statistics::calculateCost(float energy, int hour)
{
    bool isPeakHour = (hour >= peakHours.first && hour < peakHours.second);
    return energy * (isPeakHour ? tariffPeak : tariffOffpeak);
}

std::map<std::string, float> Statistics::getTodayStats()
{
    std::lock_guard<std::mutex> lock(statsMutex);
    
    std::time_t now = std::time(nullptr);
    std::tm* today = std::localtime(&now);
    char todayStr[11];
    std::strftime(todayStr, sizeof(todayStr), "%Y-%m-%d", today);
    
    std::map<std::string, float> result;
    
    if (dailyStats.find(todayStr) != dailyStats.end())
    {
        const DailyStats& stats = dailyStats[todayStr];
        result["energy_total"] = stats.energy_total;
        result["energy_peak"] = stats.energy_peak;
        result["energy_offpeak"] = stats.energy_offpeak;
        result["cost_total"] = stats.cost_total;
        result["usage_hours"] = static_cast<float>(stats.usage_hours);
        
        if (stats.energy_total > 0)
            result["avg_power"] = (stats.energy_total * 1000.0f) / stats.usage_hours;
    }
    else
    {
        result["energy_total"] = 0;
        result["energy_peak"] = 0;
        result["energy_offpeak"] = 0;
        result["cost_total"] = 0;
        result["usage_hours"] = 0;
        result["avg_power"] = 0;
    }
    
    return result;
}

std::map<std::string, float> Statistics::getYesterdayStats()
{
    std::lock_guard<std::mutex> lock(statsMutex);
    
    std::time_t now = std::time(nullptr);
    now -= 24 * 3600;
    std::tm* yesterday = std::localtime(&now);
    char yesterdayStr[11];
    std::strftime(yesterdayStr, sizeof(yesterdayStr), "%Y-%m-%d", yesterday);
    
    return getTodayStats();
}

std::map<std::string, float> Statistics::getWeekStats()
{
    std::lock_guard<std::mutex> lock(statsMutex);
    
    std::map<std::string, float> result;
    result["energy_total"] = 0;
    result["energy_peak"] = 0;
    result["energy_offpeak"] = 0;
    result["cost_total"] = 0;
    result["usage_hours"] = 0;
    result["days_count"] = 0;
    
    std::time_t now = std::time(nullptr);
    
    for (int i = 0; i < 7; i++)
    {
        std::time_t day = now - (i * 24 * 3600);
        std::tm* dayTm = std::localtime(&day);
        char dayStr[11];
        std::strftime(dayStr, sizeof(dayStr), "%Y-%m-%d", dayTm);
        
        if (dailyStats.find(dayStr) != dailyStats.end())
        {
            const DailyStats& stats = dailyStats[dayStr];
            result["energy_total"] += stats.energy_total;
            result["energy_peak"] += stats.energy_peak;
            result["energy_offpeak"] += stats.energy_offpeak;
            result["cost_total"] += stats.cost_total;
            result["usage_hours"] += stats.usage_hours;
            result["days_count"] += 1;
        }
    }
    
    if (result["days_count"] > 0)
    {
        result["energy_daily_avg"] = result["energy_total"] / result["days_count"];
        result["cost_daily_avg"] = result["cost_total"] / result["days_count"];
    }
    
    return result;
}

std::map<std::string, float> Statistics::getMonthStats()
{
    std::lock_guard<std::mutex> lock(statsMutex);
    
    std::map<std::string, float> result;
    result["energy_total"] = 0;
    result["energy_peak"] = 0;
    result["energy_offpeak"] = 0;
    result["cost_total"] = 0;
    result["usage_hours"] = 0;
    result["days_count"] = 0;
    
    std::time_t now = std::time(nullptr);
    std::tm* current = std::localtime(&now);
    int currentMonth = current->tm_mon;
    int currentYear = current->tm_year;
    
    for (const auto& pair : dailyStats)
    {
        std::tm date = {};
        std::istringstream ss(pair.first);
        ss >> std::get_time(&date, "%Y-%m-%d");
        
        if (date.tm_mon == currentMonth && date.tm_year == currentYear)
        {
            const DailyStats& stats = pair.second;
            result["energy_total"] += stats.energy_total;
            result["energy_peak"] += stats.energy_peak;
            result["energy_offpeak"] += stats.energy_offpeak;
            result["cost_total"] += stats.cost_total;
            result["usage_hours"] += stats.usage_hours;
            result["days_count"] += 1;
        }
    }
    
    return result;
}

EnergyRecord Statistics::getLatestRecord()
{
    std::lock_guard<std::mutex> lock(statsMutex);
    if (energyHistory.empty())
        return EnergyRecord{0, 0.0f, 0.0f};
    return energyHistory.back();
}

std::vector<EnergyRecord> Statistics::getHistory(int hours)
{
    std::lock_guard<std::mutex> lock(statsMutex);
    
    std::vector<EnergyRecord> result;
    uint64_t cutoff = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count() - (hours * 3600);
    
    for (const auto& record : energyHistory)
        if (record.timestamp >= cutoff)
            result.push_back(record);
    
    return result;
}

bool Statistics::exportToCSV(const std::string& filename, int days)
{
    std::lock_guard<std::mutex> lock(statsMutex);
    
    std::ofstream file(filename);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open CSV file: " + filename);
        return false;
    }
    
    file << "Date,Energy Total (kWh),Energy Peak (kWh),Energy Offpeak (kWh),"
         << "Cost Total (RUB),Usage Hours\n";
    
    for (const auto& pair : dailyStats)
    {
        const DailyStats& stats = pair.second;
        file << stats.date << ","
             << stats.energy_total << ","
             << stats.energy_peak << ","
             << stats.energy_offpeak << ","
             << stats.cost_total << ","
             << stats.usage_hours << "\n";
    }
    
    file.close();
    LOG_INFO("Statistics exported to CSV: " + filename);
    return true;
}

std::string Statistics::getJSONReport(int days)
{
#ifdef RASPBERRY_PI
    Json::Value root;
    Json::Value today(Json::objectValue);
    Json::Value week(Json::objectValue);
    Json::Value month(Json::objectValue);
    
    auto todayStats = getTodayStats();
    auto weekStats = getWeekStats();
    auto monthStats = getMonthStats();
    
    for (const auto& pair : todayStats)
        today[pair.first] = pair.second;
    
    for (const auto& pair : weekStats)
        week[pair.first] = pair.second;
    
    for (const auto& pair : monthStats)
        month[pair.first] = pair.second;

    root["today"] = today;
    root["week"] = week;
    root["month"] = month;
    root["timestamp"] = static_cast<int>(std::time(nullptr));
    
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "  ";
    return Json::writeString(builder, root);
#else
    return "";
#endif
}

void Statistics::clearHistory()
{
    std::lock_guard<std::mutex> lock(statsMutex);
    energyHistory.clear();
    LOG_INFO("Energy history cleared");
}

void Statistics::clearDailyStats()
{
    std::lock_guard<std::mutex> lock(statsMutex);
    dailyStats.clear();
    LOG_INFO("Daily statistics cleared");
}

float Statistics::calculateCO2Emissions(float energyKWh)
{
    return energyKWh * 0.33f;
}

float Statistics::calculateSavings(float energySaved)
{
    const float AVG_TARIFF = (tariffPeak + tariffOffpeak) / 2.0f;
    return energySaved * AVG_TARIFF;
}