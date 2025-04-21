#ifndef MONITOR_H
#define MONITOR_H

#include <ncurses.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <chrono>
#include <signal.h>

// Configuration structure for the activity monitor
struct MonitorConfig {
    int refresh_rate_ms = 1000;  // Update interval in milliseconds
    float cpu_threshold = 80.0f; // CPU threshold for alerts (%)
    bool show_alert = true;      // Whether to show CPU threshold alerts
    bool system_notifications = true; // Whether to show system desktop notifications
};

// Represents a single process
struct Process {
    int pid;                  // Process ID
    std::string name;         // Process name
    float cpu_percent;        // CPU usage (%)
    float mem_percent;        // Memory usage (%)
    std::string status;       // Process status (running, sleeping, etc.)
    
    // For sorting processes
    bool operator<(const Process& other) const {
        return cpu_percent > other.cpu_percent; // Default sort by CPU usage (descending)
    }
};

// Represents CPU information
struct CPUInfo {
    std::vector<float> core_usage;  // Usage per core (%)
    float total_usage;              // Total CPU usage (%)
    int num_cores;                  // Number of cores
};

// Store CPU time data for accurate calculations
struct CPUTimeInfo {
    unsigned long user;
    unsigned long nice;
    unsigned long system;
    unsigned long idle;
    unsigned long iowait;
    unsigned long irq;
    unsigned long softirq;
    unsigned long steal;
    
    // Calculate total time
    unsigned long total() const {
        return user + nice + system + idle + iowait + irq + softirq + steal;
    }
    
    // Calculate idle time
    unsigned long idle_time() const {
        return idle + iowait;
    }
    
    // Calculate active time
    unsigned long active_time() const {
        return user + nice + system + irq + softirq + steal;
    }
};

// Represents memory information
struct MemoryInfo {
    unsigned long total;      // Total memory (KB)
    unsigned long free;       // Free memory (KB)
    unsigned long available;  // Available memory (KB)
    unsigned long used;       // Used memory (KB)
    float percent_used;       // Percentage of memory used
    
    // Swap information
    unsigned long swap_total;
    unsigned long swap_free;
    unsigned long swap_used;
    float swap_percent_used;
};

// Represents disk information for each partition
struct DiskInfo {
    std::string device;           // Device name (e.g., /dev/sda1)
    std::string mount_point;      // Mount point (e.g., /)
    unsigned long total_space;    // Total space (KB)
    unsigned long free_space;     // Free space (KB)
    unsigned long used_space;     // Used space (KB)
    float percent_used;           // Percentage of space used
};

// Represents network interface information
struct NetworkInfo {
    std::string interface;    // Interface name (e.g., eth0)
    unsigned long rx_bytes;   // Received bytes
    unsigned long tx_bytes;   // Transmitted bytes
    double rx_speed;          // Download speed (Bytes/s)
    double tx_speed;          // Upload speed (Bytes/s)
};

// Main activity monitor class
class ActivityMonitor {
private:
    MonitorConfig config;
    
    // Data structures for system information
    CPUInfo cpu_info;
    MemoryInfo memory_info;
    std::vector<DiskInfo> disk_info;
    std::vector<NetworkInfo> network_info;
    std::vector<Process> processes;
    
    // Ncurses windows for different sections
    WINDOW *cpu_win;
    WINDOW *mem_win;
    WINDOW *disk_win;
    WINDOW *network_win;
    WINDOW *process_win;
    WINDOW *alert_win;
    WINDOW *confirm_win;      // Window for confirmation dialog
    
    // For calculating CPU and network usage
    std::vector<CPUTimeInfo> prev_cpu_times;
    std::vector<CPUTimeInfo> curr_cpu_times;
    std::unordered_map<std::string, std::pair<unsigned long, unsigned long>> prev_net_stats;
    
    // For process list navigation
    int process_list_offset = 0;
    int process_sort_type = 0; // 0 = CPU%, 1 = MEM%
    
    // Internal state
    bool running = true;
    std::chrono::time_point<std::chrono::high_resolution_clock> last_update;
    std::chrono::time_point<std::chrono::high_resolution_clock> last_notification;
    int terminal_height = 0;
    int terminal_width = 0;
    
    // Warning states
    bool warning_state = false;      // True if currently in warning state
    bool pre_warning_state = false;  // True if currently in pre-warning state
    
    // Private member functions
    void initializeWindows();
    void resizeWindows();
    void collectData();
    
    // Data collection methods
    void updateCPUInfo();
    void updateMemoryInfo();
    void updateDiskInfo();
    void updateNetworkInfo();
    void updateProcessInfo();
    
    // Display methods
    void displayCPUInfo();
    void displayMemoryInfo();
    void displayDiskInfo();
    void displayNetworkInfo();
    void displayProcessInfo();
    void displayAlert();
    bool displayConfirmationDialog(const std::string& message);
    
    // System notification methods
    void sendSystemNotification(const std::string& title, const std::string& message, bool critical = false);
    void checkAndSendNotifications();
    
    // Process management
    void killHighestCPUProcess();
    bool killProcess(int pid);
    
    // Helper methods
    std::string formatSize(unsigned long size_kb);
    std::string formatSpeed(double bytes_per_sec);
    std::string createBar(float percent, int width, bool use_color = true);

public:
    ActivityMonitor();
    ~ActivityMonitor();
    
    // Set configuration
    void setConfig(const MonitorConfig& new_config);
    
    // Main loop
    void run();
    
    // Handle user input
    void handleInput(int ch);
    
    // Get running state
    bool isRunning() const { return running; }
    
    // Sort processes
    void sortProcesses();
};

#endif // MONITOR_H 