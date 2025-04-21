# Activity Monitor

A terminal-based graphical system activity monitor for Ubuntu Linux, similar to htop or glances but with a custom interface.

## Features

- Real-time monitoring of system resources with a clean terminal UI
- CPU usage monitoring (total and per-core)
- Memory usage monitoring (RAM and swap)
- Disk usage monitoring (mounted partitions)
- Network usage monitoring (download/upload speeds)
- Process list with sorting by CPU or memory usage
- Advanced CPU threshold alerts with process details
- Multi-level warning system (warning and pre-warning states)
- System desktop notifications for CPU alerts
- Process management (kill high CPU processes)
- Configurable refresh rate and threshold settings

## Screenshots

```
+--------------------------------------+
| CPU Usage                            |
| Total: [||||||||||||||||||| 78.5%  ] |
| Core0: [||||||||||||||||||| 82.1%  ] |
| Core1: [||||||||||||||     67.2%  ] |
| Core2: [|||||||||||||||||| 76.3%  ] |
| Core3: [|||||||||||||||||| 79.0%  ] |
+--------------------------------------+
```

## Dependencies

- C++ compiler (g++)
- ncurses library
- notify-send command (part of libnotify-bin package)
- Standard Linux system utilities

## Building

1. Install dependencies:
   ```
   sudo apt-get update
   sudo apt-get install g++ libncurses5-dev libnotify-bin
   ```

2. Clone the repository:
   ```
   cd OSproject
   ```

3. Build the project:
   ```
   cd activity_monitor
   mkdir -p build
   make
   ```

## Usage

Run the activity monitor:

```
./activity_monitor
```

### Command-line Options

- `-r, --refresh-rate=MS`: Set refresh rate in milliseconds (default: 1000)
- `-t, --threshold=PERCENT`: Set CPU threshold for alerts (default: 80.0)
- `-a, --no-alert`: Disable CPU threshold alerts in the terminal UI
- `-n, --no-notify`: Disable system desktop notifications
- `-h, --help`: Display help information

### Keyboard Controls

- `q` or `Q`: Quit the application
- `r` or `R`: Force a refresh
- `t` or `T`: Toggle CPU threshold alerts
- `c` or `C`: Sort processes by CPU usage
- `m` or `M`: Sort processes by memory usage
- `k` or `K`: Kill the process with highest CPU usage (with confirmation)
- Arrow keys: Scroll through process list
- `Page Up`/`Page Down`: Scroll process list by pages
- `Home`/`End`: Go to beginning/end of process list

## System Notifications

The activity monitor can display system desktop notifications for CPU alerts:

1. **Pre-warning Notification**: A normal notification appears when CPU usage exceeds 80% of the set threshold, containing:
   - Current CPU usage percentage and threshold value
   - Details of the process consuming the most CPU resources
   - A message indicating CPU usage is approaching the threshold

2. **Critical Warning Notification**: An urgent notification appears when CPU usage exceeds the threshold, containing:
   - Current CPU usage compared to the threshold
   - Information about the highest CPU-consuming process
   - Instructions to press 'k' to terminate the process

Notifications are throttled to avoid flooding the desktop - they appear when the warning state changes or at most once per minute if the warning state persists.

To disable system notifications, use the `-n` or `--no-notify` command-line option.

## Warning System

The activity monitor includes a multi-level warning system:

1. **Pre-warning**: When CPU usage exceeds 80% of the set threshold, a yellow notice appears, displaying:
   - Current CPU usage and threshold value
   - Details of the process consuming the most CPU
   - A message indicating that CPU usage is approaching the threshold

2. **Critical Warning**: When CPU usage exceeds the set threshold, a red warning appears (blinking for better visibility), displaying:
   - Current CPU usage compared to the threshold
   - Detailed information about the process consuming the most CPU (PID, name, and usage %)
   - Instructions for terminating the high-CPU process

This two-level warning system helps identify potential issues before they become critical.

## Process Management

When CPU usage exceeds the configured threshold, a warning will be displayed with details of the highest CPU-consuming process. At this point, or anytime during monitoring, you can:

1. Press `k` to identify and terminate the process consuming the most CPU
2. Confirm the action in the dialog that appears
3. The process will be terminated using SIGTERM (graceful) or SIGKILL if necessary

This feature is particularly useful for quickly dealing with runaway processes or resource-intensive applications.

## Code Structure

- `main.cpp`: Entry point and command-line parsing
- `monitor.h`: Class definitions and data structures
- `monitor.cpp`: Core functionality and data collection methods
- `monitor_display.cpp`: Display rendering and UI interaction
- `system_notifications.cpp`: Desktop notification functionality

## Technical Details

- Uses `/proc/stat` for CPU information
- Uses `/proc/meminfo` for memory information
- Uses `statvfs()` for disk usage information
- Uses `/proc/net/dev` for network information
- Uses `/proc/{pid}` directories for process information
- Uses `kill()` system call for process management
- Uses `notify-send` command for system desktop notifications
- Built with ncurses for the terminal interface 