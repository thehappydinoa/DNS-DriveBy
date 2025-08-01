# Wardriving Firmware Makefile
# For DNS DriveBy hardware repurposing

# Configuration
BOARD_ESP32 = esp32dev
BOARD_ESP8266 = nodemcuv2
PORT_AUTO = $(shell ls /dev/ttyUSB* /dev/ttyACM* /dev/cu.usbserial* 2>/dev/null | head -1)
BAUD_RATE = 115200

# Default target
.PHONY: help
help:
	@echo "DNS DriveBy Wardriving Firmware"
	@echo "================================"
	@echo ""
	@echo "Targets:"
	@echo "  esp32        - Build and upload for ESP32"
	@echo "  esp8266      - Build and upload for ESP8266"
	@echo "  monitor      - Open serial monitor"
	@echo "  extract      - Extract data from device"
	@echo "  delete-data  - Delete all wardriving data"
	@echo "  stats        - Show device statistics"
	@echo "  clean        - Clean build files"
	@echo "  install-deps - Install required dependencies"
	@echo ""
	@echo "Variables:"
	@echo "  PORT=/dev/ttyUSB0  - Specify serial port"
	@echo "  BAUD=115200       - Specify baud rate"
	@echo ""
	@echo "Examples:"
	@echo "  make esp32 PORT=/dev/ttyUSB0"
	@echo "  make monitor"
	@echo "  make extract"

# Check for Arduino CLI
.PHONY: check-arduino-cli
check-arduino-cli:
	@which arduino-cli > /dev/null || (echo "Error: arduino-cli not found. Install from https://arduino.github.io/arduino-cli/" && exit 1)

# Install dependencies
.PHONY: install-deps
install-deps: check-arduino-cli
	@echo "Installing Arduino dependencies..."
	@echo "Adding ESP8266 board manager URL..."
	arduino-cli config add board_manager.additional_urls https://arduino.esp8266.com/stable/package_esp8266com_index.json || true
	arduino-cli core update-index
	arduino-cli core install esp32:esp32
	arduino-cli core install esp8266:esp8266
	arduino-cli lib install "TinyGPSPlusPlus"
	arduino-cli lib install "ESP8266 and ESP32 OLED driver for SSD1306 displays"
	arduino-cli lib install "SH1106"
	@echo "Installing Python dependencies..."
	uv sync
	@echo "Dependencies installed!"

# ESP32 build and upload
.PHONY: esp32
esp32: check-arduino-cli
	@echo "Building for ESP32..."
	@sed -i.bak 's|^//\s*#define BOARD_ESP32|#define BOARD_ESP32|' Wardriving.ino
	arduino-cli compile -b esp32:esp32:esp32dev Wardriving.ino
	@if [ -n "$(PORT)" ]; then \
		echo "Uploading to $(PORT)..."; \
		arduino-cli upload -b esp32:esp32:esp32dev --port $(PORT) Wardriving.ino; \
	elif [ -n "$(PORT_AUTO)" ]; then \
		echo "Auto-detected port: $(PORT_AUTO)"; \
		arduino-cli upload -b esp32:esp32:esp32dev --port $(PORT_AUTO) Wardriving.ino; \
	else \
		echo "No port specified and none auto-detected. Use: make esp32 PORT=/dev/ttyUSB0"; \
	fi

# Update firmware version with git hash
.PHONY: update-version
update-version:
	@echo "Updating firmware version with git hash..."
	@python3 update_version.py

# ESP8266 build and upload
.PHONY: esp8266
esp8266: check-arduino-cli update-version
	@echo "Building for ESP8266..."
	@sed -i.bak 's|^#define BOARD_ESP32|//#define BOARD_ESP32|' Wardriving.ino
	arduino-cli compile -b esp8266:esp8266:d1_mini Wardriving.ino
	@if [ -n "$(PORT)" ]; then \
		echo "Uploading to $(PORT)..."; \
		arduino-cli upload -b esp8266:esp8266:d1_mini --port $(PORT) Wardriving.ino; \
	elif [ -n "$(PORT_AUTO)" ]; then \
		echo "Auto-detected port: $(PORT_AUTO)"; \
		arduino-cli upload -b esp8266:esp8266:d1_mini --port $(PORT_AUTO) Wardriving.ino; \
	else \
		echo "No port specified and none auto-detected. Use: make esp8266 PORT=/dev/ttyUSB0"; \
		echo "If you get permission errors, try: sudo make esp8266 PORT=/dev/ttyUSB0"; \
	fi

# ESP8266 Compile only
.PHONY: esp8266-compile
esp8266-compile: check-arduino-cli update-version
	@echo "Building for ESP8266..."
	@sed -i.bak 's|^#define BOARD_ESP32|//#define BOARD_ESP32|' Wardriving.ino
	arduino-cli compile -b esp8266:esp8266:d1_mini Wardriving.ino

# ESP8266 Upload only
.PHONY: esp8266-upload
esp8266-upload: check-arduino-cli
	@if [ -n "$(PORT)" ]; then \
		echo "Uploading to $(PORT)..."; \
		arduino-cli upload -b esp8266:esp8266:d1_mini --port $(PORT) Wardriving.ino; \
	elif [ -n "$(PORT_AUTO)" ]; then \
		echo "Auto-detected port: $(PORT_AUTO)"; \
		arduino-cli upload -b esp8266:esp8266:d1_mini --port $(PORT_AUTO) Wardriving.ino; \
	else \
		echo "No port specified and none auto-detected. Use: make esp8266 PORT=/dev/ttyUSB0"; \
		echo "If you get permission errors, try: sudo make esp8266 PORT=/dev/ttyUSB0"; \
	fi

# Serial monitor
.PHONY: monitor
monitor:
	@if [ -n "$(PORT)" ]; then \
		echo "Opening serial monitor on $(PORT) at $(BAUD_RATE) baud..."; \
		arduino-cli monitor --port $(PORT) -c baudrate=$(BAUD_RATE); \
	elif [ -n "$(PORT_AUTO)" ]; then \
		echo "Auto-detected port: $(PORT_AUTO)"; \
		arduino-cli monitor --port $(PORT_AUTO) -c baudrate=$(BAUD_RATE); \
	else \
		echo "No port specified and none auto-detected. Use: make monitor PORT=/dev/ttyUSB0"; \
	fi

# Extract data from device
.PHONY: extract
extract:
	@echo "Extracting wardriving data..."
	@if [ -n "$(PORT)" ]; then \
		uv run python3 data_extractor.py -p $(PORT); \
	else \
		uv run python3 data_extractor.py; \
	fi

# Extract data with sudo (for permission issues)
.PHONY: extract-sudo
extract-sudo:
	@echo "Extracting wardriving data with sudo..."
	@if [ -n "$(PORT)" ]; then \
		sudo /root/.local/bin/uv run python3 data_extractor.py -p $(PORT); \
	else \
		sudo /root/.local/bin/uv run python3 data_extractor.py; \
	fi

# Show statistics only
.PHONY: stats
stats:
	@echo "Getting wardriving statistics..."
	@if [ -n "$(PORT)" ]; then \
		uv run python3 data_extractor.py -p $(PORT) -s; \
	else \
		uv run python3 data_extractor.py -s; \
	fi

# Show statistics with sudo
.PHONY: stats-sudo
stats-sudo:
	@echo "Getting wardriving statistics with sudo..."
	@if [ -n "$(PORT)" ]; then \
		sudo /root/.local/bin/uv run python3 data_extractor.py -p $(PORT) -s; \
	else \
		sudo /root/.local/bin/uv run python3 data_extractor.py -s; \
	fi

# Delete all wardriving data
.PHONY: delete-data
delete-data:
	@echo "Deleting all wardriving data..."
	@if [ -n "$(PORT)" ]; then \
		echo "DELETE_DATA" | sudo tee $(PORT) > /dev/null; \
		echo "Data deletion command sent to $(PORT)"; \
		echo "Check monitor to confirm deletion"; \
	else \
		echo "No port specified. Use: make delete-data PORT=/dev/ttyUSB0"; \
	fi

# Delete data and show confirmation
.PHONY: delete-data-monitor
delete-data-monitor:
	@echo "Deleting data and showing confirmation..."
	@if [ -n "$(PORT)" ]; then \
		echo "DELETE_DATA" | sudo tee $(PORT) > /dev/null; \
		echo "Data deletion command sent. Opening monitor to confirm..."; \
		sleep 2; \
		sudo make monitor PORT=$(PORT); \
	else \
		echo "No port specified. Use: make delete-data-monitor PORT=/dev/ttyUSB0"; \
	fi

# Clean build files and temporary data
.PHONY: clean
clean:
	@echo "Cleaning build files and temporary data..."
	@rm -rf build/
	@rm -f *.bak
	@rm -rf __pycache__/
	@rm -f *.pyc
	@echo "Clean complete!"

# Flash SPIFFS filesystem (if needed)
.PHONY: spiffs-esp32
spiffs-esp32:
	@echo "Uploading SPIFFS for ESP32..."
	@if [ -n "$(PORT)" ]; then \
		arduino-cli upload -p $(PORT) --fqbn $(BOARD_ESP32) --input-dir build/spiffs; \
	else \
		echo "No port specified. Use: make spiffs-esp32 PORT=/dev/ttyUSB0"; \
	fi

# Development helpers
.PHONY: dev-setup
dev-setup: install-deps
	@echo "Setting up development environment..."
	@echo "Creating example configuration..."
	@cp Wardriving.ino Wardriving.ino.example
	@echo "Development setup complete!"

# Backup current data
.PHONY: backup
backup:
	@echo "Backing up current wardriving data..."
	@mkdir -p backups
	@timestamp=$$(date +%Y%m%d_%H%M%S); \
	uv run python3 data_extractor.py -o "backups/wardrive_backup_$$timestamp.csv" $(if $(PORT),-p $(PORT))
	@echo "Backup complete!"

# Backup current data with sudo
.PHONY: backup-sudo
backup-sudo:
	@echo "Backing up current wardriving data with sudo..."
	@mkdir -p backups
	@timestamp=$$(date +%Y%m%d_%H%M%S); \
	sudo /root/.local/bin/uv run python3 data_extractor.py -o "backups/wardrive_backup_$$timestamp.csv" $(if $(PORT),-p $(PORT))
	@echo "Backup complete!"

# Format data for WiGLE upload
.PHONY: wigle-format
wigle-format:
	@echo "Extracting data in WiGLE format..."
	@uv run python3 data_extractor.py -o "wigle_upload_$$(date +%Y%m%d_%H%M%S).csv" $(if $(PORT),-p $(PORT))
	@echo "WiGLE format file ready for upload!"

# Format data for WiGLE upload with sudo
.PHONY: wigle-format-sudo
wigle-format-sudo:
	@echo "Extracting data in WiGLE format with sudo..."
	@sudo /root/.local/bin/uv run python3 data_extractor.py -o "wigle_upload_$$(date +%Y%m%d_%H%M%S).csv" $(if $(PORT),-p $(PORT))
	@echo "WiGLE format file ready for upload!"

# Test connection to device
.PHONY: test-connection
test-connection:
	@echo "Testing connection to device..."
	@if [ -n "$(PORT)" ]; then \
		echo "Testing $(PORT)..."; \
		uv run python3 -c "import serial; s=serial.Serial('$(PORT)', $(BAUD_RATE), timeout=2); print('Connection OK' if s.is_open else 'Connection Failed'); s.close()"; \
	else \
		echo "No port specified. Use: make test-connection PORT=/dev/ttyUSB0"; \
	fi

# Test connection to device with sudo
.PHONY: test-connection-sudo
test-connection-sudo:
	@echo "Testing connection to device with sudo..."
	@if [ -n "$(PORT)" ]; then \
		echo "Testing $(PORT)..."; \
		sudo /root/.local/bin/uv run python3 -c "import serial; s=serial.Serial('$(PORT)', $(BAUD_RATE), timeout=2); print('Connection OK' if s.is_open else 'Connection Failed'); s.close()"; \
	else \
		echo "No port specified. Use: make test-connection-sudo PORT=/dev/ttyUSB0"; \
	fi

# Show device info
.PHONY: device-info
device-info:
	@echo "Scanning for ESP devices..."
	@for port in /dev/ttyUSB* /dev/ttyACM* /dev/cu.usbserial* /dev/cu.SLAB_USBtoUART*; do \
		if [ -e "$$port" ]; then \
			echo "Found potential device: $$port"; \
		fi; \
	done

# Complete deployment pipeline
.PHONY: deploy-esp32
deploy-esp32: esp32 monitor

.PHONY: deploy-esp8266  
deploy-esp8266: esp8266 monitor

# Show help
.PHONY: help
help:
	@echo "DNS-DriveBy Wardriving Makefile"
	@echo ""
	@echo "Build and Upload:"
	@echo "  make esp8266 PORT=/dev/ttyUSB0    - Build and upload to ESP8266"
	@echo "  make esp32 PORT=/dev/ttyUSB0      - Build and upload to ESP32"
	@echo "  make esp8266-compile              - Compile only (ESP8266)"
	@echo "  make esp32-compile                - Compile only (ESP32)"
	@echo ""
	@echo "Data Extraction (Regular):"
	@echo "  make extract PORT=/dev/ttyUSB0    - Extract wardriving data"
	@echo "  make stats PORT=/dev/ttyUSB0      - Show statistics only"
	@echo "  make backup PORT=/dev/ttyUSB0     - Backup data with timestamp"
	@echo "  make wigle-format PORT=/dev/ttyUSB0 - Format for WiGLE upload"
	@echo ""
	@echo "Data Extraction (with sudo):"
	@echo "  make extract-sudo PORT=/dev/ttyUSB0    - Extract with sudo"
	@echo "  make stats-sudo PORT=/dev/ttyUSB0      - Stats with sudo"
	@echo "  make backup-sudo PORT=/dev/ttyUSB0     - Backup with sudo"
	@echo "  make wigle-format-sudo PORT=/dev/ttyUSB0 - WiGLE format with sudo"
	@echo ""
	@echo "Device Management:"
	@echo "  make monitor PORT=/dev/ttyUSB0    - Open serial monitor"
	@echo "  make delete-data PORT=/dev/ttyUSB0 - Delete all wardriving data"
	@echo "  make test-connection PORT=/dev/ttyUSB0 - Test device connection"
	@echo "  make test-connection-sudo PORT=/dev/ttyUSB0 - Test connection with sudo"
	@echo "  make device-info                   - Scan for ESP devices"
	@echo ""
	@echo "Development:"
	@echo "  make clean                         - Clean build artifacts"
	@echo "  make update-version               - Update firmware version"
	@echo "  make install-deps                 - Install Python dependencies"
	@echo ""
	@echo "Examples:"
	@echo "  make esp8266 PORT=/dev/ttyUSB0"
	@echo "  make extract-sudo PORT=/dev/ttyUSB0"
	@echo "  sudo make esp8266 PORT=/dev/ttyUSB0  # If you get permission errors" 