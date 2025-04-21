.PHONY: all clean run

all:
	@echo "Building activity monitor..."
	@cd activity_monitor && make

clean:
	@echo "Cleaning activity monitor..."
	@cd activity_monitor && make clean

run: all
	@echo "Running activity monitor..."
	@./activity_monitor/activity_monitor

help:
	@echo "Activity Monitor Makefile"
	@echo "-------------------------"
	@echo "make       - Build the project"
	@echo "make clean - Clean build files"
	@echo "make run   - Build and run the activity monitor"
	@echo "make help  - Show this help message" 