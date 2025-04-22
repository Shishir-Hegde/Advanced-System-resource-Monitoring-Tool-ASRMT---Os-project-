#include "../include/monitor.h"
#include <iostream>
#include <getopt.h>

// Show usage info
void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [OPTIONS]\n"
              << "Terminal-based activity monitor for system resources.\n\n"
              << "Options:\n"
              << "  -r, --refresh-rate=MS    Set refresh rate in milliseconds (default: 1000)\n"
              << "  -t, --threshold=PERCENT  Set CPU threshold for alerts (default: 80.0)\n"
              << "  -a, --no-alert           Disable CPU threshold alerts\n"
              << "  -n, --no-notify          Disable system desktop notifications\n"
              << "  -d, --debug              Enable debug output\n"
              << "  -o, --debug-only         Run in debug-only mode (no UI)\n"
              << "  -h, --help               Display this help and exit\n"
              << std::endl;
}

// Main entry point
int main(int argc, char* argv[]) {
    MonitorConfig config;
    
    static struct option long_options[] = {
        {"refresh-rate", required_argument, 0, 'r'},
        {"threshold",    required_argument, 0, 't'},
        {"no-alert",     no_argument,       0, 'a'},
        {"no-notify",    no_argument,       0, 'n'},
        {"debug",        no_argument,       0, 'd'},
        {"debug-only",   no_argument,       0, 'o'},
        {"help",         no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "r:t:andoh", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'r':
                config.refresh_rate_ms = std::stoi(optarg);
                if (config.refresh_rate_ms < 100) {
                    std::cerr << "Warning: Refresh rate too low. Setting to 100ms minimum." << std::endl;
                    config.refresh_rate_ms = 100;
                }
                break;
            case 't':
                config.cpu_threshold = std::stof(optarg);
                if (config.cpu_threshold < 0.0f || config.cpu_threshold > 100.0f) {
                    std::cerr << "Warning: Threshold must be between 0 and 100. Using default of 80%." << std::endl;
                    config.cpu_threshold = 80.0f;
                }
                break;
            case 'a':
                config.show_alert = false;
                break;
            case 'n':
                config.system_notifications = false;
                break;
            case 'd':
                config.debug_mode = true;
                break;
            case 'o':
                config.debug_mode = true;
                config.debug_only_mode = true;
                break;
            case 'h':
                printUsage(argv[0]);
                return 0;
            default:
                printUsage(argv[0]);
                return 1;
        }
    }
    
    try {
        ActivityMonitor monitor;
        monitor.setConfig(config);
        
        if (config.debug_only_mode) {
            monitor.runDebugMode();
        } else {
            monitor.run();
        }
    } catch (const std::exception& e) {
        if (!config.debug_only_mode) {
            endwin();
        }
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 