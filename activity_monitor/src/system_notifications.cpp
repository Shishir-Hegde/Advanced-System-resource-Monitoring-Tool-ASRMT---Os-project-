#include "../include/monitor.h"
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <chrono>

// Send a system notification using notify-send (part of libnotify-bin)
void ActivityMonitor::sendSystemNotification(const std::string& title, const std::string& message, bool critical) {
    // Use notify-send command which is available on most Ubuntu systems
    std::string urgency = critical ? "critical" : "normal";
    std::string icon = critical ? "dialog-warning" : "dialog-information";
    
    // Escape special characters in title and message for shell command
    auto escapeShell = [](const std::string& s) {
        std::string result = s;
        size_t pos = 0;
        while ((pos = result.find("\"", pos)) != std::string::npos) {
            result.replace(pos, 1, "\\\"");
            pos += 2;
        }
        return result;
    };
    
    std::string escaped_title = escapeShell(title);
    std::string escaped_message = escapeShell(message);
    
    // Build the command
    std::string cmd = "notify-send -u " + urgency + " -i " + icon + 
                      " \"" + escaped_title + "\" \"" + escaped_message + "\"";
    
    // Execute the command
    int ret = system(cmd.c_str());
    
    // Ignore return value as we don't want to break the application if notification fails
    (void)ret;
}

// Check CPU usage and send system notifications if necessary
void ActivityMonitor::checkAndSendNotifications() {
    if (!config.system_notifications) {
        return;  // System notifications are disabled
    }
    
    // Define thresholds
    float pre_warning_threshold = config.cpu_threshold * 0.8f;
    bool should_warn = cpu_info.total_usage > config.cpu_threshold;
    bool should_pre_warn = !should_warn && cpu_info.total_usage > pre_warning_threshold;
    
    // Get highest CPU process if available
    Process* top_process = nullptr;
    if (!processes.empty()) {
        top_process = &processes[0];
    }
    
    // Get current time for notification throttling
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_notification).count();
    
    // Only send a notification if state changed or if it's been at least 60 seconds since the last one
    bool state_changed = (should_warn != warning_state) || (should_pre_warn != pre_warning_state);
    
    if (state_changed || elapsed >= 60) {
        if (should_warn) {
            // Critical warning - over threshold
            std::ostringstream title_oss;
            title_oss << "CPU Usage Critical: " << std::fixed << std::setprecision(1) 
                    << cpu_info.total_usage << "% (Threshold: " << config.cpu_threshold << "%)";
            
            std::ostringstream msg_oss;
            if (top_process != nullptr) {
                msg_oss << "Highest CPU process: " << top_process->pid << " (" 
                        << top_process->name << ") using " << std::fixed 
                        << std::setprecision(1) << top_process->cpu_percent << "% CPU\n\n"
                        << "Press 'k' in the activity monitor to terminate this process.";
            } else {
                msg_oss << "No specific process identified as the main consumer.";
            }
            
            sendSystemNotification(title_oss.str(), msg_oss.str(), true);
            last_notification = now;
        } 
        else if (should_pre_warn) {
            // Pre-warning - approaching threshold
            std::ostringstream title_oss;
            title_oss << "CPU Usage Warning: " << std::fixed << std::setprecision(1) 
                    << cpu_info.total_usage << "% (Threshold: " << config.cpu_threshold << "%)";
            
            std::ostringstream msg_oss;
            msg_oss << "CPU utilization is approaching threshold!\n";
            
            if (top_process != nullptr) {
                msg_oss << "Highest CPU process: " << top_process->pid << " (" 
                        << top_process->name << ") using " << std::fixed 
                        << std::setprecision(1) << top_process->cpu_percent << "% CPU";
            }
            
            sendSystemNotification(title_oss.str(), msg_oss.str(), false);
            last_notification = now;
        }
    }
    
    // Update state
    warning_state = should_warn;
    pre_warning_state = should_pre_warn;
} 