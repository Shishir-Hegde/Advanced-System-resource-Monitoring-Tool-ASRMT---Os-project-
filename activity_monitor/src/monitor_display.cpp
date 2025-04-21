#include "../include/monitor.h"
#include <sstream>
#include <iomanip>
#include <thread>

// Display CPU information
void ActivityMonitor::displayCPUInfo() {
    wclear(cpu_win);
    box(cpu_win, 0, 0);
    
    // Get window size
    int height, width;
    getmaxyx(cpu_win, height, width);
    
    // Draw header
    wattron(cpu_win, COLOR_PAIR(5));
    mvwprintw(cpu_win, 0, 2, " CPU Usage ");
    wattroff(cpu_win, COLOR_PAIR(5));
    
    // Draw total CPU usage bar
    mvwprintw(cpu_win, 1, 2, "Total:");
    
    // Choose color based on usage
    int color = 1; // default green
    if (cpu_info.total_usage > config.cpu_threshold) {
        color = 3; // red for over threshold
    } else if (cpu_info.total_usage > 60.0f) {
        color = 2; // yellow for medium usage
    }
    
    wattron(cpu_win, COLOR_PAIR(color));
    std::string bar = createBar(cpu_info.total_usage, width - 10, false);
    mvwprintw(cpu_win, 1, 10, "%s", bar.c_str());
    wattroff(cpu_win, COLOR_PAIR(color));
    
    // Draw CPU cores
    int cores_to_show = std::min(static_cast<int>(cpu_info.core_usage.size()), height - 3);
    for (int i = 0; i < cores_to_show; i++) {
        float usage = cpu_info.core_usage[i];
        
        // Choose color based on usage
        color = 1; // default green
        if (usage > config.cpu_threshold) {
            color = 3; // red for over threshold
        } else if (usage > 60.0f) {
            color = 2; // yellow for medium usage
        }
        
        mvwprintw(cpu_win, i + 2, 2, "Core%2d:", i);
        wattron(cpu_win, COLOR_PAIR(color));
        bar = createBar(usage, width - 10, false);
        mvwprintw(cpu_win, i + 2, 10, "%s", bar.c_str());
        wattroff(cpu_win, COLOR_PAIR(color));
    }
    
    wrefresh(cpu_win);
}

// Display memory information
void ActivityMonitor::displayMemoryInfo() {
    wclear(mem_win);
    box(mem_win, 0, 0);
    
    // Get window size
    int width;
    getmaxyx(mem_win, std::ignore, width);
    
    // Draw header
    wattron(mem_win, COLOR_PAIR(5));
    mvwprintw(mem_win, 0, 2, " Memory Performance ");
    wattroff(mem_win, COLOR_PAIR(5));
    
    // Choose color based on usage
    int color = 1; // default green
    if (memory_info.percent_used > 90.0f) {
        color = 3; // red for high usage
    } else if (memory_info.percent_used > 70.0f) {
        color = 2; // yellow for medium usage
    }
    
    // Print RAM usage
    mvwprintw(mem_win, 2, 2, "RAM:");
    wattron(mem_win, COLOR_PAIR(color));
    std::string bar = createBar(memory_info.percent_used, width - 8, false);
    mvwprintw(mem_win, 2, 8, "%s", bar.c_str());
    wattroff(mem_win, COLOR_PAIR(color));
    
    // Print RAM details
    std::string total = formatSize(memory_info.total);
    std::string used = formatSize(memory_info.used);
    std::string free = formatSize(memory_info.available);
    std::string cached = formatSize(memory_info.cached);
    std::string buffers = formatSize(memory_info.buffers);
    
    mvwprintw(mem_win, 3, 2, "Total: %s", total.c_str());
    mvwprintw(mem_win, 4, 2, "Used : %s", used.c_str());
    mvwprintw(mem_win, 5, 2, "Free : %s", free.c_str());
    
    // Performance metrics section with highlight box
    wattron(mem_win, COLOR_PAIR(5));
    mvwprintw(mem_win, 6, 2, "===== Performance Metrics =====");
    wattroff(mem_win, COLOR_PAIR(5));
    
    mvwprintw(mem_win, 7, 2, "Cache: %s", cached.c_str());
    mvwprintw(mem_win, 8, 2, "Buffr: %s", buffers.c_str());
    
    // Display memory performance metrics with enhanced visibility
    if (memory_info.cache_hit_rate > 0) {
        // Calculate color based on hit rate
        int hit_color = 1; // default green
        if (memory_info.cache_hit_rate < 80.0f) {
            hit_color = 3; // red for low hit rate
        } else if (memory_info.cache_hit_rate < 90.0f) {
            hit_color = 2; // yellow for medium hit rate
        }
        
        wattron(mem_win, COLOR_PAIR(hit_color) | A_BOLD);
        mvwprintw(mem_win, 9, 2, "Hit Rate: %.1f%%", memory_info.cache_hit_rate);
        wattroff(mem_win, COLOR_PAIR(hit_color) | A_BOLD);
        
        // Draw a small hit rate bar
        int hit_width = 20;
        int filled = static_cast<int>(hit_width * memory_info.cache_hit_rate / 100.0);
        mvwprintw(mem_win, 9, 18, "[");
        wattron(mem_win, COLOR_PAIR(hit_color));
        for (int i = 0; i < hit_width; i++) {
            mvwaddch(mem_win, 9, 19 + i, (i < filled) ? '|' : ' ');
        }
        wattroff(mem_win, COLOR_PAIR(hit_color));
        mvwprintw(mem_win, 9, 19 + hit_width, "]");
    } else {
        mvwprintw(mem_win, 9, 2, "Hit Rate: N/A");
    }
    
    // Display memory latency with enhanced visibility
    std::string latency = formatLatency(memory_info.latency_ns, true);
    
    // Choose color based on latency (lower is better)
    int latency_color = 1; // default green
    if (memory_info.latency_ns > 100.0f) {
        latency_color = 3; // red for high latency
    } else if (memory_info.latency_ns > 80.0f) {
        latency_color = 2; // yellow for medium latency
    }
    
    wattron(mem_win, COLOR_PAIR(latency_color) | A_BOLD);
    mvwprintw(mem_win, 10, 2, "Latency: %s", latency.c_str());
    wattroff(mem_win, COLOR_PAIR(latency_color) | A_BOLD);
    
    // Print swap usage if available
    if (memory_info.swap_total > 0) {
        wattron(mem_win, COLOR_PAIR(5));
        mvwprintw(mem_win, 12, 2, "===== Swap Memory =====");
        wattroff(mem_win, COLOR_PAIR(5));
        
        // Choose color based on swap usage
        color = 1; // default green
        if (memory_info.swap_percent_used > 50.0f) {
            color = 3; // red for high usage
        } else if (memory_info.swap_percent_used > 25.0f) {
            color = 2; // yellow for medium usage
        }
        
        mvwprintw(mem_win, 13, 2, "Swap:");
        wattron(mem_win, COLOR_PAIR(color));
        bar = createBar(memory_info.swap_percent_used, width - 8, false);
        mvwprintw(mem_win, 13, 8, "%s", bar.c_str());
        wattroff(mem_win, COLOR_PAIR(color));
        
        std::string swap_total = formatSize(memory_info.swap_total);
        std::string swap_used = formatSize(memory_info.swap_used);
        std::string swap_free = formatSize(memory_info.swap_free);
        
        mvwprintw(mem_win, 14, 2, "Total: %s", swap_total.c_str());
        mvwprintw(mem_win, 15, 2, "Used : %s", swap_used.c_str());
        mvwprintw(mem_win, 16, 2, "Free : %s", swap_free.c_str());
    }
    
    wrefresh(mem_win);
}

// Display disk information
void ActivityMonitor::displayDiskInfo() {
    wclear(disk_win);
    box(disk_win, 0, 0);
    
    // Get window size
    int height, width;
    getmaxyx(disk_win, height, width);
    
    // Draw header
    wattron(disk_win, COLOR_PAIR(5));
    mvwprintw(disk_win, 0, 2, " Disk Performance ");
    wattroff(disk_win, COLOR_PAIR(5));
    
    // Show disk information
    int max_disks = height - 4; // Reduced to allow for header and footer
    int disks_shown = 0;
    
    // Draw header row
    wattron(disk_win, A_BOLD);
    mvwprintw(disk_win, 1, 2, "Mount      Usage    Read Latency");
    wattroff(disk_win, A_BOLD);
    
    for (size_t i = 0; i < disk_info.size() && disks_shown < max_disks; i++) {
        const DiskInfo& disk = disk_info[i];
        
        // Create a shorter mount point display
        std::string mount = disk.mount_point;
        if (mount.length() > 8) {
            mount = mount.substr(0, 7) + "+";
        }
        
        // Choose color based on usage
        int color = 1; // default green
        if (disk.percent_used > 90.0f) {
            color = 3; // red for high usage
        } else if (disk.percent_used > 70.0f) {
            color = 2; // yellow for medium usage
        }
        
        // Show disk information
        mvwprintw(disk_win, disks_shown + 2, 2, "%-8s", mount.c_str());
        
        // Create a visual percentage bar (now with more space for read latency)
        wattron(disk_win, COLOR_PAIR(color));
        std::string bar = createBar(disk.percent_used, 20, false);
        mvwprintw(disk_win, disks_shown + 2, 11, "%s", bar.c_str());
        wattroff(disk_win, COLOR_PAIR(color));
        
        // Show read latency information
        std::string read_latency = formatLatency(disk.read_latency_ms, false);
        
        // Choose color based on read latency
        int read_color = 1; // default green
        if (disk.read_latency_ms > 30.0f) {
            read_color = 3; // red for high latency
        } else if (disk.read_latency_ms > 10.0f) {
            read_color = 2; // yellow for medium latency
        }
        
        wattron(disk_win, COLOR_PAIR(read_color) | A_BOLD);
        mvwprintw(disk_win, disks_shown + 2, 36, "%-12s", read_latency.c_str());
        wattroff(disk_win, COLOR_PAIR(read_color) | A_BOLD);
        
        disks_shown++;
    }
    
    // Add a simple key/legend at the bottom
    if (height > 6) {
        wattron(disk_win, A_BOLD);
        mvwprintw(disk_win, height - 2, 2, "Latency Key:");
        wattroff(disk_win, A_BOLD);
        
        wattron(disk_win, COLOR_PAIR(1));
        mvwprintw(disk_win, height - 2, 15, "Good");
        wattroff(disk_win, COLOR_PAIR(1));
        
        wattron(disk_win, COLOR_PAIR(2));
        mvwprintw(disk_win, height - 2, 25, "Medium");
        wattroff(disk_win, COLOR_PAIR(2));
        
        wattron(disk_win, COLOR_PAIR(3));
        mvwprintw(disk_win, height - 2, 37, "High/Poor");
        wattroff(disk_win, COLOR_PAIR(3));
    }
    
    wrefresh(disk_win);
}

// Display process information
void ActivityMonitor::displayProcessInfo() {
    wclear(process_win);
    box(process_win, 0, 0);
    
    // Get window size
    int height, width;
    getmaxyx(process_win, height, width);
    
    // Draw header
    wattron(process_win, COLOR_PAIR(5));
    mvwprintw(process_win, 0, 2, " Processes (Press 'c' for CPU sort, 'm' for memory sort, 'k' to kill highest CPU process) ");
    wattroff(process_win, COLOR_PAIR(5));
    
    // Draw column headers
    wattron(process_win, A_BOLD);
    mvwprintw(process_win, 1, 2, "%-6s %-25s %-10s %-10s", 
              "PID", "Name", "CPU%", "Memory%");
    wattroff(process_win, A_BOLD);
    
    // Calculate how many processes we can show
    int process_rows = height - 3;
    int end_index = std::min(static_cast<int>(processes.size()), 
                             process_list_offset + process_rows);
    
    // Draw processes
    for (int i = process_list_offset; i < end_index; i++) {
        const Process& proc = processes[i];
        int row = i - process_list_offset + 2;
        
        // Choose color based on CPU usage
        int color = 1; // default green
        if (proc.cpu_percent > config.cpu_threshold / 2) {
            color = 3; // red for high usage
        } else if (proc.cpu_percent > config.cpu_threshold / 4) {
            color = 2; // yellow for medium usage
        }
        
        wattron(process_win, COLOR_PAIR(color));
        
        // Create a truncated name if necessary
        std::string disp_name;
        if (proc.name.length() > 25) {
            disp_name = proc.name.substr(0, 22) + "...";
        } else {
            disp_name = proc.name;
        }
        
        // Draw process information - removed status column
        mvwprintw(process_win, row, 2, "%-6d %-25s %6.1f%%     %6.1f%%", 
                  proc.pid, 
                  disp_name.c_str(),
                  proc.cpu_percent,
                  proc.mem_percent);
        
        wattroff(process_win, COLOR_PAIR(color));
    }
    
    // Show a scroll indicator if there are more processes
    if (static_cast<int>(processes.size()) > process_rows) {
        double percent = static_cast<double>(process_list_offset) / 
                         (processes.size() - process_rows);
        int scrollbar_pos = 2 + static_cast<int>((height - 4) * percent);
        
        for (int i = 2; i < height - 1; i++) {
            if (i == scrollbar_pos) {
                mvwaddch(process_win, i, width - 2, '#');
            } else {
                mvwaddch(process_win, i, width - 2, '|');
            }
        }
    }
    
    wrefresh(process_win);
}

// Display CPU alert when threshold is exceeded
void ActivityMonitor::displayAlert() {
    // Check if we need to display alert
    float pre_warning_threshold = config.cpu_threshold * 0.8f;
    bool is_warning = cpu_info.total_usage > config.cpu_threshold;
    bool is_pre_warning = !is_warning && config.show_alert && cpu_info.total_usage > pre_warning_threshold;
    
    if (!config.show_alert || (!is_warning && !is_pre_warning)) {
        // Delete alert window if it exists and is not needed
        if (alert_win != nullptr) {
            delwin(alert_win);
            alert_win = nullptr;
            // Redraw all windows to clear alert
            displayCPUInfo();
            displayMemoryInfo();
            displayDiskInfo();
            displayProcessInfo();
        }
        return;
    }
    
    // Make sure processes are sorted by CPU usage
    if (process_sort_type != 0) {
        process_sort_type = 0;
        sortProcesses();
    }
    
    // Get highest CPU process if available
    Process* top_process = nullptr;
    if (!processes.empty()) {
        top_process = &processes[0];
    }
    
    // Create alert window if it doesn't exist
    if (alert_win == nullptr) {
        int height = 9;  // Increased height for top process details
        int width = 60;  // Increased width for more text
        int start_y = (terminal_height - height) / 2;
        int start_x = (terminal_width - width) / 2;
        
        alert_win = newwin(height, width, start_y, start_x);
    }
    
    // Get window width
    int width;
    getmaxyx(alert_win, std::ignore, width);
    
    // Get current time to create blinking effect
    auto now = std::chrono::system_clock::now();
    auto time_point = std::chrono::system_clock::to_time_t(now);
    bool blink = (time_point % 2 == 0);
    
    // Display alert
    wclear(alert_win);
    
    if (is_warning) {
        // Critical warning - over threshold
        if (blink) {
            wbkgd(alert_win, COLOR_PAIR(3)); // Red background
        } else {
            wbkgd(alert_win, COLOR_PAIR(0));
            box(alert_win, 0, 0);
        }
        
        // Warning title
        std::string title = " WARNING: High CPU Usage ";
        wattron(alert_win, A_BOLD);
        mvwprintw(alert_win, 0, (width - title.length()) / 2, "%s", title.c_str());
        wattroff(alert_win, A_BOLD);
        
        std::ostringstream oss;
        oss << "CPU Usage: " << std::fixed << std::setprecision(1) 
            << cpu_info.total_usage << "% > " << config.cpu_threshold << "%";
        
        int center_pos = (width - oss.str().length()) / 2;
        mvwprintw(alert_win, 2, center_pos, "%s", oss.str().c_str());
        
        // Add top process details if available
        if (top_process != nullptr) {
            std::ostringstream proc_oss;
            proc_oss << "Highest CPU process: " << top_process->pid << " (" 
                    << top_process->name << ") using " << std::fixed 
                    << std::setprecision(1) << top_process->cpu_percent << "% CPU";
            
            std::string proc_info = proc_oss.str();
            if (proc_info.length() > static_cast<size_t>(width - 4)) {
                proc_info = proc_info.substr(0, width - 7) + "...";
            }
            
            mvwprintw(alert_win, 4, (width - proc_info.length()) / 2, "%s", proc_info.c_str());
        }
        
        // Add instruction for killing the highest CPU process
        std::string instruction = "Press 'k' to kill highest CPU process";
        mvwprintw(alert_win, 6, (width - instruction.length()) / 2, "%s", instruction.c_str());
    } else {
        // Pre-warning - approaching threshold
        wbkgd(alert_win, COLOR_PAIR(0));
        box(alert_win, 0, 0);
        wattron(alert_win, COLOR_PAIR(2)); // Yellow for pre-warning
        
        // Pre-warning title
        std::string title = " NOTICE: Approaching CPU Threshold ";
        wattron(alert_win, A_BOLD);
        mvwprintw(alert_win, 0, (width - title.length()) / 2, "%s", title.c_str());
        wattroff(alert_win, A_BOLD);
        
        std::ostringstream oss;
        oss << "CPU Usage: " << std::fixed << std::setprecision(1) 
            << cpu_info.total_usage << "% (Threshold: " << config.cpu_threshold << "%)";
        
        int center_pos = (width - oss.str().length()) / 2;
        mvwprintw(alert_win, 2, center_pos, "%s", oss.str().c_str());
        
        // Add top process details if available
        if (top_process != nullptr) {
            std::ostringstream proc_oss;
            proc_oss << "Highest CPU process: " << top_process->pid << " (" 
                    << top_process->name << ") using " << std::fixed 
                    << std::setprecision(1) << top_process->cpu_percent << "% CPU";
            
            std::string proc_info = proc_oss.str();
            if (proc_info.length() > static_cast<size_t>(width - 4)) {
                proc_info = proc_info.substr(0, width - 7) + "...";
            }
            
            mvwprintw(alert_win, 4, (width - proc_info.length()) / 2, "%s", proc_info.c_str());
        }
        
        std::string approaching_msg = "CPU utilization is approaching threshold!";
        mvwprintw(alert_win, 6, (width - approaching_msg.length()) / 2, "%s", approaching_msg.c_str());
        
        wattroff(alert_win, COLOR_PAIR(2));
    }
    
    wrefresh(alert_win);
}

// Display a confirmation dialog and return the user's choice
bool ActivityMonitor::displayConfirmationDialog(const std::string& message) {
    // Create confirmation window
    int height = 7;
    int width = 60;
    int start_y = (terminal_height - height) / 2;
    int start_x = (terminal_width - width) / 2;
    
    WINDOW* dialog = newwin(height, width, start_y, start_x);
    box(dialog, 0, 0);
    
    // Draw header
    wattron(dialog, COLOR_PAIR(5));
    mvwprintw(dialog, 0, 2, " Confirmation ");
    wattroff(dialog, COLOR_PAIR(5));
    
    // Draw message
    mvwprintw(dialog, 2, (width - message.length()) / 2, "%s", message.c_str());
    
    // Draw options
    std::string options = "Press 'y' to confirm, 'n' to cancel";
    mvwprintw(dialog, 4, (width - options.length()) / 2, "%s", options.c_str());
    
    wrefresh(dialog);
    
    // Wait for user input
    int ch;
    bool result = false;
    bool waiting = true;
    
    while (waiting) {
        ch = getch();
        switch (ch) {
            case 'y':
            case 'Y':
                result = true;
                waiting = false;
                break;
            case 'n':
            case 'N':
            case 27:  // ESC key
                result = false;
                waiting = false;
                break;
        }
    }
    
    // Clean up
    delwin(dialog);
    
    // Redraw all windows
    displayCPUInfo();
    displayMemoryInfo();
    displayDiskInfo();
    displayProcessInfo();
    if (alert_win != nullptr) {
        displayAlert();
    }
    
    return result;
}

// Kill a process with the given PID
bool ActivityMonitor::killProcess(int pid) {
    // Check if the PID is valid
    if (pid <= 0) {
        return false;
    }
    
    // Send SIGTERM signal to the process
    int result = kill(pid, SIGTERM);
    
    // If SIGTERM fails, try SIGKILL
    if (result != 0) {
        result = kill(pid, SIGKILL);
    }
    
    // Update process list
    collectData();
    
    return (result == 0);
}

// Find and kill the process with the highest CPU usage
void ActivityMonitor::killHighestCPUProcess() {
    // Make sure processes are sorted by CPU usage (descending)
    if (process_sort_type != 0) {
        process_sort_type = 0;
        sortProcesses();
    }
    
    // Check if there are any processes
    if (processes.empty()) {
        return;
    }
    
    // Get the process with the highest CPU usage
    const Process& top_process = processes[0];
    
    // Create confirmation message
    std::ostringstream oss;
    oss << "Kill process " << top_process.pid << " (" << top_process.name 
        << ") using " << std::fixed << std::setprecision(1) << top_process.cpu_percent << "% CPU?";
    
    // Ask for confirmation
    if (displayConfirmationDialog(oss.str())) {
        // Kill the process
        if (killProcess(top_process.pid)) {
            // Process killed successfully, refresh data
            collectData();
        }
    }
}

// Handle user input
void ActivityMonitor::handleInput(int ch) {
    switch (ch) {
        case 'q':
        case 'Q':
            // Quit the application
            running = false;
            break;
        
        case 'r':
        case 'R':
            // Force a refresh
            collectData();
            break;
        
        case 't':
        case 'T':
            // Toggle alert display
            config.show_alert = !config.show_alert;
            break;
        
        case 'c':
        case 'C':
            // Sort by CPU usage
            process_sort_type = 0;
            sortProcesses();
            break;
        
        case 'm':
        case 'M':
            // Sort by memory usage
            process_sort_type = 1;
            sortProcesses();
            break;
            
        case 'k':
        case 'K':
            // Kill highest CPU process
            killHighestCPUProcess();
            break;
        
        case KEY_UP:
            // Scroll process list up
            if (process_list_offset > 0) {
                process_list_offset--;
            }
            break;
        
        case KEY_DOWN:
            // Scroll process list down
            if (process_list_offset < static_cast<int>(processes.size() - 1)) {
                process_list_offset++;
            }
            break;
        
        case KEY_PPAGE:
            // Page up
            process_list_offset = std::max(0, process_list_offset - 10);
            break;
        
        case KEY_NPAGE:
            // Page down
            process_list_offset = std::min(static_cast<int>(processes.size() - 1), 
                                          process_list_offset + 10);
            break;
        
        case KEY_HOME:
            // Go to top of process list
            process_list_offset = 0;
            break;
        
        case KEY_END:
            // Go to end of process list
            process_list_offset = std::max(0, static_cast<int>(processes.size() - 1));
            break;
    }
}

// Main run loop
void ActivityMonitor::run() {
    // Initial data collection
    collectData();
    
    while (running) {
        // Check for terminal resize
        resizeWindows();
        
        // Display data
        displayCPUInfo();
        displayMemoryInfo();
        displayDiskInfo();
        displayProcessInfo();
        displayAlert();
        
        // Check and send system notifications if needed
        checkAndSendNotifications();
        
        // Handle user input
        int ch = getch();
        if (ch != ERR) {
            handleInput(ch);
        }
        
        // Check if it's time to update data
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update);
        
        if (elapsed.count() >= config.refresh_rate_ms) {
            collectData();
            last_update = now;
        }
        
        // Sleep to avoid using too much CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
} 