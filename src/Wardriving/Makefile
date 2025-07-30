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
	arduino-cli core update-index
	arduino-cli core install esp32:esp32
	arduino-cli core install esp8266:esp8266
	arduino-cli lib install "TinyGPS++"
	arduino-cli lib install "ESP8266 and ESP32 OLED driver for SSD1306 displays"
	@echo "Installing Python dependencies..."
	pip3 install pyserial
	@echo "Dependencies installed!"

# ESP32 build and upload
.PHONY: esp32
esp32: check-arduino-cli
	@echo "Building for ESP32..."
	@sed -i.bak 's|^//\s*#define BOARD_ESP32|#define BOARD_ESP32|' Wardriving.ino
	arduino-cli compile --fqbn $(BOARD_ESP32) Wardriving.ino
	@if [ -n "$(PORT)" ]; then \
		echo "Uploading to $(PORT)..."; \
		arduino-cli upload -p $(PORT) --fqbn $(BOARD_ESP32) Wardriving.ino; \
	elif [ -n "$(PORT_AUTO)" ]; then \
		echo "Auto-detected port: $(PORT_AUTO)"; \
		arduino-cli upload -p $(PORT_AUTO) --fqbn $(BOARD_ESP32) Wardriving.ino; \
	else \
		echo "No port specified and none auto-detected. Use: make esp32 PORT=/dev/ttyUSB0"; \
	fi

# ESP8266 build and upload
.PHONY: esp8266
esp8266: check-arduino-cli
	@echo "Building for ESP8266..."
	@sed -i.bak 's|^#define BOARD_ESP32|//#define BOARD_ESP32|' Wardriving.ino
	arduino-cli compile --fqbn $(BOARD_ESP8266) Wardriving.ino
	@if [ -n "$(PORT)" ]; then \
		echo "Uploading to $(PORT)..."; \
		arduino-cli upload -p $(PORT) --fqbn $(BOARD_ESP8266) Wardriving.ino; \
	elif [ -n "$(PORT_AUTO)" ]; then \
		echo "Auto-detected port: $(PORT_AUTO)"; \
		arduino-cli upload -p $(PORT_AUTO) --fqbn $(BOARD_ESP8266) Wardriving.ino; \
	else \
		echo "No port specified and none auto-detected. Use: make esp8266 PORT=/dev/ttyUSB0"; \
	fi

# Serial monitor
.PHONY: monitor
monitor:
	@if [ -n "$(PORT)" ]; then \
		echo "Opening serial monitor on $(PORT) at $(BAUD_RATE) baud..."; \
		arduino-cli monitor -p $(PORT) -c baudrate=$(BAUD_RATE); \
	elif [ -n "$(PORT_AUTO)" ]; then \
		echo "Auto-detected port: $(PORT_AUTO)"; \
		arduino-cli monitor -p $(PORT_AUTO) -c baudrate=$(BAUD_RATE); \
	else \
		echo "No port specified and none auto-detected. Use: make monitor PORT=/dev/ttyUSB0"; \
	fi

# Extract data from device
.PHONY: extract
extract:
	@echo "Extracting wardriving data..."
	@if [ -n "$(PORT)" ]; then \
		python3 data_extractor.py -p $(PORT); \
	else \
		python3 data_extractor.py; \
	fi

# Show statistics only
.PHONY: stats
stats:
	@echo "Getting wardriving statistics..."
	@if [ -n "$(PORT)" ]; then \
		python3 data_extractor.py -p $(PORT) -s; \
	else \
		python3 data_extractor.py -s; \
	fi

# Clean build files
.PHONY: clean
clean:
	@echo "Cleaning build files..."
	@rm -rf build/
	@rm -f *.bak
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
	python3 data_extractor.py -o "backups/wardrive_backup_$$timestamp.csv" $(if $(PORT),-p $(PORT))
	@echo "Backup complete!"

# Format data for WiGLE upload
.PHONY: wigle-format
wigle-format:
	@echo "Extracting data in WiGLE format..."
	@python3 data_extractor.py -o "wigle_upload_$$(date +%Y%m%d_%H%M%S).csv" $(if $(PORT),-p $(PORT))
	@echo "WiGLE format file ready for upload!"

# Test connection to device
.PHONY: test-connection
test-connection:
	@echo "Testing connection to device..."
	@if [ -n "$(PORT)" ]; then \
		echo "Testing $(PORT)..."; \
		python3 -c "import serial; s=serial.Serial('$(PORT)', $(BAUD_RATE), timeout=2); print('Connection OK' if s.is_open else 'Connection Failed'); s.close()"; \
	else \
		echo "No port specified. Use: make test-connection PORT=/dev/ttyUSB0"; \
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