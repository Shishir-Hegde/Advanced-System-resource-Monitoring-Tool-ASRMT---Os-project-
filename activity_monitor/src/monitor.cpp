#include "../include/monitor.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <dirent.h>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <thread>
#include <sys/statvfs.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <iostream>
#include <sys/types.h>

// Constructor
ActivityMonitor::ActivityMonitor() {
    // Set default values
    last_update = std::chrono::high_resolution_clock::now();
    last_notification = last_update;  // Initialize notification timer
    
    // Until setConfig is called, we don't know if we're in debug-only mode
}

// Destructor
ActivityMonitor::~ActivityMonitor() {
    // Close debug file if open
    if (debug_file.is_open()) {
        debug_file.close();
    }
    
    // Only clean up ncurses if we're not in debug-only mode
    if (!config.debug_only_mode) {
        // Clean up windows
        delwin(cpu_win);
        delwin(mem_win);
        delwin(disk_win);
        delwin(process_win);
        
        if (alert_win != nullptr) {
            delwin(alert_win);
        }
        
        // End ncurses
        endwin();
    }
}

// Initialize the windows
void ActivityMonitor::initializeWindows() {
    int height, width;
    getmaxyx(stdscr, height, width);
    
    // Calculate dimensions for each window
    int cpu_height = height / 4;
    int mem_height = height / 4;
    int disk_height = height / 4;
    int process_height = height / 2;
    
    // Create windows
    cpu_win = newwin(cpu_height, width, 0, 0);
    mem_win = newwin(mem_height, width / 2, cpu_height, 0);
    disk_win = newwin(disk_height, width / 2, cpu_height, width / 2);
    process_win = newwin(process_height, width, height - process_height, 0);
    
    // Create alert window (initially hidden)
    alert_win = nullptr;
    
    // Enable scrolling for process window
    scrollok(process_win, TRUE);
}

// Resize windows when terminal size changes
void ActivityMonitor::resizeWindows() {
    int new_height, new_width;
    getmaxyx(stdscr, new_height, new_width);
    
    // Check if terminal size has changed
    if (new_height != terminal_height || new_width != terminal_width) {
        terminal_height = new_height;
        terminal_width = new_width;
        
        // Delete old windows
        delwin(cpu_win);
        delwin(mem_win);
        delwin(disk_win);
        delwin(process_win);
        
        if (alert_win != nullptr) {
            delwin(alert_win);
            alert_win = nullptr;
        }
        
        // Recreate windows with new dimensions
        initializeWindows();
        
        // Force full redraw
        clear();
        refresh();
    }
}

// Set configuration
void ActivityMonitor::setConfig(const MonitorConfig& new_config) {
    config = new_config;
    
    // Initialize ncurses if not in debug-only mode
    if (!config.debug_only_mode) {
        initscr();
        start_color();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(0);  // Hide cursor
        timeout(0);   // Non-blocking input
        
        // Get terminal dimensions
        getmaxyx(stdscr, terminal_height, terminal_width);
        
        // Initialize colors
        init_pair(1, COLOR_GREEN, COLOR_BLACK);    // Normal (green)
        init_pair(2, COLOR_YELLOW, COLOR_BLACK);   // Warning (yellow)
        init_pair(3, COLOR_RED, COLOR_BLACK);      // Critical (red)
        init_pair(4, COLOR_CYAN, COLOR_BLACK);     // Info (cyan)
        init_pair(5, COLOR_WHITE, COLOR_BLUE);     // Headers (white on blue)
        
        // Initialize windows
        initializeWindows();
    }
    
    // Initialize CPU data
    updateCPUInfo();
    
    // If debug mode is enabled, log this event
    if (config.debug_mode) {
        debugLog("Debug mode enabled");
        debugLog("Configuration: ");
        debugLog("  Refresh rate: " + std::to_string(config.refresh_rate_ms) + " ms");
        debugLog("  CPU threshold: " + std::to_string(config.cpu_threshold) + "%");
        debugLog("  Show alerts: " + std::string(config.show_alert ? "true" : "false"));
        debugLog("  System notifications: " + std::string(config.system_notifications ? "true" : "false"));
        debugLog("  Debug-only mode: " + std::string(config.debug_only_mode ? "true" : "false"));
    }
}

// Sort processes based on current sort type
void ActivityMonitor::sortProcesses() {
    if (process_sort_type == 0) {
        // Sort by CPU usage (descending)
        std::sort(processes.begin(), processes.end(), 
            [](const Process& a, const Process& b) { 
                return a.cpu_percent > b.cpu_percent; 
            });
    } else {
        // Sort by Memory usage (descending)
        std::sort(processes.begin(), processes.end(), 
            [](const Process& a, const Process& b) { 
                return a.mem_percent > b.mem_percent; 
            });
    }
}

// Helper method to format size with appropriate units
std::string ActivityMonitor::formatSize(unsigned long size_kb) {
    std::ostringstream oss;
    
    if (size_kb < 1024) {
        oss << size_kb << " KB";
    } else if (size_kb < 1024 * 1024) {
        oss << std::fixed << std::setprecision(1) << (size_kb / 1024.0) << " MB";
    } else {
        oss << std::fixed << std::setprecision(2) << (size_kb / (1024.0 * 1024.0)) << " GB";
    }
    
    return oss.str();
}

// Helper method to format latency (memory in ns, disk in ms)
std::string ActivityMonitor::formatLatency(float latency, bool is_memory) {
    std::ostringstream oss;
    
    if (latency < 0) {
        // Latency is not available
        return "N/A";
    }
    
    oss << std::fixed << std::setprecision(2);
    
    if (is_memory) {
        // Memory latency in nanoseconds
        if (latency < 1000) {
            oss << latency << " ns";
        } else {
            oss << (latency / 1000.0) << " μs";
        }
    } else {
        // Disk latency in milliseconds
        if (latency < 1.0) {
            oss << (latency * 1000.0) << " μs";
        } else if (latency < 1000.0) {
            oss << latency << " ms";
        } else {
            oss << (latency / 1000.0) << " s";
        }
    }
    
    return oss.str();
}

// Create a progress bar
std::string ActivityMonitor::createBar(float percent, int width, bool use_color) {
    // use_color parameter is currently unused, but kept for future enhancements
    (void)use_color; // Suppress unused parameter warning
    
    int bar_width = width - 7; // Leave space for percentage
    int fill_width = static_cast<int>(bar_width * percent / 100.0);
    
    std::string bar = "[";
    for (int i = 0; i < bar_width; i++) {
        if (i < fill_width) {
            bar += "|";
        } else {
            bar += " ";
        }
    }
    bar += "]";
    
    // Add percentage
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << percent << "%";
    std::string percent_str = oss.str();
    
    // Insert percentage in the middle of the bar
    int pos = bar.length() / 2 - percent_str.length() / 2;
    bar.replace(pos, percent_str.length(), percent_str);
    
    return bar;
}

// Collect all system data
void ActivityMonitor::collectData() {
    updateCPUInfo();
    updateMemoryInfo();
    updateDiskInfo();
    updateProcessInfo();
    updateMemoryStats();
    updateDiskLatency();
}

// Update CPU information by reading /proc/stat
void ActivityMonitor::updateCPUInfo() {
    std::ifstream stat_file("/proc/stat");
    if (!stat_file.is_open()) {
        throw std::runtime_error("Failed to open /proc/stat");
    }
    
    // Store previous CPU times for calculation
    prev_cpu_times = curr_cpu_times;
    curr_cpu_times.clear();
    
    std::string line;
    size_t core_count = 0;
    std::vector<float> core_percentages;
    
    while (std::getline(stat_file, line)) {
        if (line.substr(0, 3) == "cpu") {
            std::istringstream iss(line);
            std::string cpu_label;
            iss >> cpu_label;
            
            // Parse CPU times
            CPUTimeInfo cpu_time;
            iss >> cpu_time.user >> cpu_time.nice >> cpu_time.system >> cpu_time.idle 
                >> cpu_time.iowait >> cpu_time.irq >> cpu_time.softirq >> cpu_time.steal;
            
            // Add this CPU time info to our current dataset
            curr_cpu_times.push_back(cpu_time);
            
            // If we have previous data, calculate CPU usage percentage
            if (!prev_cpu_times.empty() && prev_cpu_times.size() > core_count) {
                const CPUTimeInfo& prev = prev_cpu_times[core_count];
                const CPUTimeInfo& curr = cpu_time;
                
                // Calculate the deltas
                unsigned long total_delta = curr.total() - prev.total();
                unsigned long idle_delta = curr.idle_time() - prev.idle_time();
                
                if (total_delta > 0) {
                    // The formula: CPU usage = 100% - (idle_delta / total_delta * 100%)
                    float cpu_percentage = 100.0f * (1.0f - static_cast<float>(idle_delta) / total_delta);
                    
                    // For the first line (total CPU), update total_usage
                    if (cpu_label == "cpu") {
                        cpu_info.total_usage = cpu_percentage;
                    } else {
                        core_percentages.push_back(cpu_percentage);
                    }
                }
            }
            
            core_count++;
        } else if (line.substr(0, 4) != "cpu") {
            // If we've processed all CPU lines, break
            break;
        }
    }
    
    // Update core count and usage data
    cpu_info.num_cores = static_cast<int>(core_count) - 1;  // Subtract 1 for the total "cpu" line
    cpu_info.core_usage = core_percentages;
}

// Update memory information by reading /proc/meminfo
void ActivityMonitor::updateMemoryInfo() {
    std::ifstream meminfo_file("/proc/meminfo");
    if (!meminfo_file.is_open()) {
        throw std::runtime_error("Failed to open /proc/meminfo");
    }
    
    std::string line;
    unsigned long mem_total = 0, mem_free = 0, mem_available = 0;
    unsigned long swap_total = 0, swap_free = 0;
    unsigned long cached = 0, buffers = 0;
    
    while (std::getline(meminfo_file, line)) {
        std::istringstream iss(line);
        std::string key;
        unsigned long value;
        std::string unit;
        
        iss >> key >> value >> unit;
        
        if (key == "MemTotal:") {
            mem_total = value;
        } else if (key == "MemFree:") {
            mem_free = value;
        } else if (key == "MemAvailable:") {
            mem_available = value;
        } else if (key == "SwapTotal:") {
            swap_total = value;
        } else if (key == "SwapFree:") {
            swap_free = value;
        } else if (key == "Cached:") {
            cached = value;
        } else if (key == "Buffers:") {
            buffers = value;
        }
    }
    
    // Calculate used memory and percentages
    unsigned long mem_used = mem_total - mem_available;
    float mem_percent = (mem_total > 0) ? (100.0f * mem_used / mem_total) : 0.0f;
    
    unsigned long swap_used = swap_total - swap_free;
    float swap_percent = (swap_total > 0) ? (100.0f * swap_used / swap_total) : 0.0f;
    
    // Update memory info structure
    memory_info.total = mem_total;
    memory_info.free = mem_free;
    memory_info.available = mem_available;
    memory_info.used = mem_used;
    memory_info.percent_used = mem_percent;
    
    memory_info.swap_total = swap_total;
    memory_info.swap_free = swap_free;
    memory_info.swap_used = swap_used;
    memory_info.swap_percent_used = swap_percent;
    
    memory_info.cached = cached;
    memory_info.buffers = buffers;
}

// Update disk information using statvfs
void ActivityMonitor::updateDiskInfo() {
    // Read /proc/mounts to get mounted filesystems
    std::ifstream mounts_file("/proc/mounts");
    if (!mounts_file.is_open()) {
        throw std::runtime_error("Failed to open /proc/mounts");
    }
    
    disk_info.clear();
    
    std::string line;
    while (std::getline(mounts_file, line)) {
        std::istringstream iss(line);
        std::string device, mount_point, fs_type, options;
        int dump, pass;
        
        iss >> device >> mount_point >> fs_type >> options >> dump >> pass;
        
        // Skip non-physical filesystems
        if (fs_type == "proc" || fs_type == "sysfs" || fs_type == "devpts" || 
            fs_type == "tmpfs" || fs_type == "devtmpfs" || fs_type == "debugfs" ||
            mount_point.substr(0, 4) == "/sys" || mount_point.substr(0, 5) == "/proc" ||
            mount_point.substr(0, 4) == "/dev" || mount_point.substr(0, 4) == "/run") {
            continue;
        }
        
        // Get disk usage information
        struct statvfs stat;
        if (statvfs(mount_point.c_str(), &stat) != 0) {
            continue;  // Skip if we can't get stats
        }
        
        DiskInfo info;
        info.device = device;
        info.mount_point = mount_point;
        
        // Calculate sizes in KB
        const unsigned long block_size = stat.f_frsize;
        info.total_space = (stat.f_blocks * block_size) / 1024;
        info.free_space = (stat.f_bfree * block_size) / 1024;
        info.used_space = info.total_space - info.free_space;
        
        // Calculate percentage
        if (info.total_space > 0) {
            info.percent_used = 100.0f * static_cast<float>(info.used_space) / info.total_space;
        } else {
            info.percent_used = 0.0f;
        }
        
        disk_info.push_back(info);
    }
}

// Update process information by scanning /proc directory
void ActivityMonitor::updateProcessInfo() {
    processes.clear();
    
    // Open the /proc directory
    DIR* proc_dir = opendir("/proc");
    if (proc_dir == nullptr) {
        throw std::runtime_error("Failed to open /proc directory");
    }
    
    // Get total system memory for percentage calculations
    unsigned long total_memory = memory_info.total;
    
    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != nullptr) {
        // Check if the entry is a directory and name is a number (PID)
        if (entry->d_type == DT_DIR) {
            std::string name = entry->d_name;
            bool is_pid = true;
            for (char c : name) {
                if (!std::isdigit(c)) {
                    is_pid = false;
                    break;
                }
            }
            
            if (!is_pid) {
                continue;
            }
            
            int pid = std::stoi(name);
            
            // Get process status
            std::string status_path = "/proc/" + name + "/status";
            std::ifstream status_file(status_path);
            if (!status_file.is_open()) {
                continue;  // Process might have terminated
            }
            
            Process proc;
            proc.pid = pid;
            proc.name = "unknown";
            proc.cpu_percent = 0.0f;
            proc.mem_percent = 0.0f;
            
            // Read status file
            std::string line;
            unsigned long vm_rss = 0;
            
            while (std::getline(status_file, line)) {
                if (line.compare(0, 5, "Name:") == 0) {
                    proc.name = line.substr(6);
                    // Trim whitespace
                    proc.name.erase(0, proc.name.find_first_not_of(" \t"));
                    proc.name.erase(proc.name.find_last_not_of(" \t") + 1);
                } else if (line.compare(0, 6, "VmRSS:") == 0) {
                    std::istringstream iss(line.substr(6));
                    iss >> vm_rss;
                }
            }
            
            // Calculate memory percentage
            if (total_memory > 0) {
                proc.mem_percent = 100.0f * static_cast<float>(vm_rss) / total_memory;
            }
            
            // Read process stat for CPU usage
            std::string stat_path = "/proc/" + name + "/stat";
            std::ifstream stat_file(stat_path);
            if (stat_file.is_open()) {
                std::string content;
                std::getline(stat_file, content);
                std::istringstream iss(content);
                
                // Skip PID and name fields
                int dummy_pid;
                std::string dummy_name;
                iss >> dummy_pid >> dummy_name;
                
                // Skip to utime and stime (fields 14 and 15)
                std::string dummy;
                for (int i = 0; i < 11; i++) {
                    iss >> dummy;
                }
                
                unsigned long utime, stime;
                iss >> utime >> stime;
                
                // Simple approximation of CPU usage
                // This isn't completely accurate but gives a rough estimate
                // For better accuracy, we'd need to track process CPU time between updates
                unsigned long total_time = utime + stime;
                proc.cpu_percent = 0.1f * total_time / (cpu_info.num_cores * 100.0f);
                
                if (config.debug_mode) {
                    debugLog("Process " + std::to_string(proc.pid) + " (" + proc.name + ") CPU calculation:");
                    debugLog("  utime: " + std::to_string(utime) + ", stime: " + std::to_string(stime));
                    debugLog("  total_time: " + std::to_string(total_time));
                    debugLog("  num_cores: " + std::to_string(cpu_info.num_cores));
                    debugLog("  cpu_percent: " + std::to_string(proc.cpu_percent));
                }
            }
            
            // Add process to list
            processes.push_back(proc);
        }
    }
    
    closedir(proc_dir);
    
    // Sort processes
    sortProcesses();
}

// Debug log method
void ActivityMonitor::debugLog(const std::string& message) {
    if (config.debug_mode) {
        // Open the file if it's not open yet
        if (!debug_file.is_open()) {
            debug_file.open("activity_monitor_debug.log", std::ios::out | std::ios::app);
            // Add timestamp for session start
            auto now = std::chrono::system_clock::now();
            auto now_time_t = std::chrono::system_clock::to_time_t(now);
            debug_file << "\n\n----- Debug session started at " << std::ctime(&now_time_t) << "-----\n";
        }
        
        // Write the message to file
        debug_file << message << std::endl;
        debug_file.flush(); // Ensure it's written immediately
        
        // Also write to stderr
        std::cerr << "DEBUG: " << message << std::endl;
    }
}

// Update memory cache hit rates and latency metrics
void ActivityMonitor::updateMemoryStats() {
    // Read cached and buffers memory amounts from /proc/meminfo (already done in updateMemoryInfo)
    std::ifstream meminfo_file("/proc/meminfo");
    if (meminfo_file.is_open()) {
        std::string line;
        while (std::getline(meminfo_file, line)) {
            std::istringstream iss(line);
            std::string key;
            unsigned long value;
            std::string unit;
            
            iss >> key >> value >> unit;
            
            if (key == "Cached:") {
                memory_info.cached = value;
            } else if (key == "Buffers:") {
                memory_info.buffers = value;
            }
        }
    }
    
    // Calculate cache hit rate - this is a simplified approximation
    // In a real system, this would come from performance counters
    if (memory_info.total > 0) {
        // Calculate cached memory percentage
        float cache_percentage = 100.0f * (memory_info.cached + memory_info.buffers) / memory_info.total;
        
        // Simulate cache hit rate based on cache size (simplified model)
        // More cache generally means higher hit rates
        memory_info.cache_hit_rate = 70.0f + (cache_percentage * 0.25f);
        
        // Cap at 99% maximum hit rate
        if (memory_info.cache_hit_rate > 99.0f) {
            memory_info.cache_hit_rate = 99.0f;
        }
    } else {
        memory_info.cache_hit_rate = -1.0f;
    }
    
    // Estimate memory latency - this would ideally come from hardware counters
    // For simulation purposes, we're generating a realistic value
    // Typical DDR4 memory latency is around 60-100ns
    memory_info.latency_ns = 60.0f + (40.0f * memory_info.percent_used / 100.0f);
    
    if (config.debug_mode) {
        debugLog("Memory cache hit rate: " + std::to_string(memory_info.cache_hit_rate) + "%");
        debugLog("Memory latency: " + formatLatency(memory_info.latency_ns, true));
    }
}

// Update disk I/O and latency metrics
void ActivityMonitor::updateDiskLatency() {
    // Read disk stats from /proc/diskstats
    std::ifstream diskstats_file("/proc/diskstats");
    if (!diskstats_file.is_open()) {
        if (config.debug_mode) {
            debugLog("Failed to open /proc/diskstats");
        }
        return;
    }
    
    // Create a map for easier lookup of disk information by device name
    std::unordered_map<std::string, DiskInfo*> disk_lookup;
    for (auto& disk : disk_info) {
        // Extract the device name without path (e.g., "sda1" from "/dev/sda1")
        size_t pos = disk.device.rfind('/');
        std::string dev_name = (pos != std::string::npos) ? disk.device.substr(pos + 1) : disk.device;
        disk_lookup[dev_name] = &disk;
        
        // Initialize latency metrics
        disk.read_latency_ms = -1.0f;
    }
    
    // Parse disk stats
    std::string line;
    while (std::getline(diskstats_file, line)) {
        std::istringstream iss(line);
        int major, minor;
        std::string dev_name;
        unsigned long reads, reads_merged, sectors_read, read_ms;
        unsigned long writes, writes_merged, sectors_written, write_ms;
        unsigned long ios_in_progress, io_ms, weighted_io_ms;
        
        // Parse disk stats line
        iss >> major >> minor >> dev_name 
            >> reads >> reads_merged >> sectors_read >> read_ms
            >> writes >> writes_merged >> sectors_written >> write_ms
            >> ios_in_progress >> io_ms >> weighted_io_ms;
        
        // Check if this device is one we're monitoring
        if (disk_lookup.find(dev_name) != disk_lookup.end()) {
            DiskInfo* disk = disk_lookup[dev_name];
            
            // Calculate latency metrics
            if (reads > 0) {
                disk->read_latency_ms = static_cast<float>(read_ms) / reads;
            }
            
            // Store total I/O operations
            disk->io_operations = reads + writes;
            
            if (config.debug_mode) {
                debugLog("Disk " + dev_name + " read latency: " + formatLatency(disk->read_latency_ms, false));
                debugLog("Disk " + dev_name + " I/O operations: " + std::to_string(disk->io_operations));
            }
        }
    }
}

// Run in debug-only mode (no UI)
void ActivityMonitor::runDebugMode() {
    // Initialize necessary data
    updateCPUInfo();
    updateMemoryInfo();
    updateDiskInfo();
    updateProcessInfo();
    updateMemoryStats();
    updateDiskLatency();
    
    // Print initial debug information
    debugLog("===== Starting debug-only mode =====");
    debugLog("System information:");
    debugLog("  CPU cores: " + std::to_string(cpu_info.num_cores));
    debugLog("  Total memory: " + formatSize(memory_info.total));
    debugLog("  Memory cache hit rate: " + std::to_string(memory_info.cache_hit_rate) + "%");
    debugLog("  Memory latency: " + formatLatency(memory_info.latency_ns, true));
    
    // Run for specified number of cycles
    int cycles = 10; // Collect data for 10 cycles
    
    for (int i = 0; i < cycles && running; i++) {
        debugLog("===== Collecting data (cycle " + std::to_string(i+1) + "/" + std::to_string(cycles) + ") =====");
        
        // Update data
        updateCPUInfo();
        debugLog("CPU usage: " + std::to_string(cpu_info.total_usage) + "%");
        
        updateMemoryInfo();
        updateMemoryStats();
        debugLog("Memory usage: " + std::to_string(memory_info.percent_used) + "% (" + formatSize(memory_info.used) + "/" + formatSize(memory_info.total) + ")");
        debugLog("Cache hit rate: " + std::to_string(memory_info.cache_hit_rate) + "%, Latency: " + formatLatency(memory_info.latency_ns, true));
        
        // Log disk information
        updateDiskLatency();
        debugLog("Disk information:");
        for (const auto& disk : disk_info) {
            debugLog("  " + disk.mount_point + " (" + disk.device + "): " + 
                     std::to_string(disk.percent_used) + "% used, Read latency: " + 
                     formatLatency(disk.read_latency_ms, false));
        }
        
        updateProcessInfo();
        debugLog("Found " + std::to_string(processes.size()) + " processes");
        
        // Log the top 5 CPU-consuming processes
        sortProcesses();
        debugLog("Top CPU-consuming processes:");
        int count = std::min(5, static_cast<int>(processes.size()));
        for (int j = 0; j < count; j++) {
            const Process& proc = processes[j];
            debugLog("  [" + std::to_string(j+1) + "] PID: " + std::to_string(proc.pid) + 
                     ", Name: " + proc.name + 
                     ", CPU: " + std::to_string(proc.cpu_percent) + "%");
        }
        
        // Wait for the next update
        std::this_thread::sleep_for(std::chrono::milliseconds(config.refresh_rate_ms));
    }
    
    debugLog("===== Debug-only mode completed =====");
} 