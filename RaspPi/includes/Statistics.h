#pragma once

#include <vector>
#include <map>
#include <string>
#include <chrono>
#include <mutex>

struct EnergyRecord
{
    uint64_t timestamp;
    float energy;
    float cost;
};

struct DailyStats
{
    std::string date;
    float energy_total;
    float energy_peak;
    float energy_offpeak;
    float cost_total;
    int usage_hours;
};

class Statistics
{
private:    
    void updateDailyStats(const EnergyRecord& record);
    float calculateCost(float energy, int hour);
    
public:
    Statistics();
    
    void setTariffs(float peak, float offpeak);
    void setPeakHours(int start, int end);
    
    void addEnergyReading(float energy);
    void addPowerReading(float power, int durationSeconds);
    
    std::map<std::string, float> getTodayStats();
    std::map<std::string, float> getYesterdayStats();
    std::map<std::string, float> getWeekStats();
    std::map<std::string, float> getMonthStats();
    
    EnergyRecord getLatestRecord();
    std::vector<EnergyRecord> getHistory(int hours = 24);
    
    bool exportToCSV(const std::string& filename, int days = 30);
    std::string getJSONReport(int days = 7);
    
    void clearHistory();
    void clearDailyStats();
    
    float calculateCO2Emissions(float energyKWh);
    float calculateSavings(float energySaved);

private:
    std::vector<EnergyRecord> energyHistory;
    std::map<std::string, DailyStats> dailyStats;
    
    std::mutex statsMutex;
    
    float tariffPeak {5.0f};
    float tariffOffpeak {2.0f};
    std::pair<int, int> peakHours;
};