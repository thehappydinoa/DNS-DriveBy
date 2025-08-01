# DNS DriveBy Wardriving Firmware

This firmware repurposes your DNS DriveBy hardware for WiFi and Bluetooth wardriving, generating data compatible with WiGLE and other mapping platforms.

## 🚀 Quick Start

### 1. Hardware Setup
Your DNS DriveBy device should have:
- ✅ ESP32/ESP8266 microcontroller
- ✅ GPS module (working and getting signal)
- ✅ SH1106 OLED display (optional - see SD Card section)
- ✅ Battery/power source
- ✅ USB cable for programming

### 2. Software Setup
```bash
# Install dependencies
cd src/Wardriving
make install-deps

# Configure for your board
# Edit Wardriving.ino:
#   ESP32: Keep #define BOARD_ESP32 uncommented
#   ESP8266: Comment out #define BOARD_ESP32
```

### 3. Upload Firmware
```bash
# For ESP32
make esp32 PORT=/dev/ttyUSB0

# For ESP8266  
make esp8266 PORT=/dev/ttyUSB0

# 🚨 Upload Troubleshooting: If upload fails, double-click the button on your board quickly, then run upload again
```

### 4. Start Wardriving
```bash
# Monitor device
make monitor PORT=/dev/ttyUSB0

# Extract data when ready
make extract PORT=/dev/ttyUSB0

# Get statistics
make stats PORT=/dev/ttyUSB0
```

## 📋 Features

- **WiFi Scanning**: Detects all WiFi networks with MAC, SSID, encryption, signal strength, and location
- **Bluetooth Scanning**: Detects Bluetooth devices (ESP32 only)
- **GPS Integration**: Records precise coordinates and accurate UTC timestamps
- **WiGLE Format**: Outputs data in standard WiGLE CSV format
- **Real-time Display**: Shows scanning progress and statistics (when display enabled)
- **Smart Storage**: Automatic warnings and storage management
- **SD Card Support**: Unlimited storage with SD card (enabled by default)

## 🔧 Hardware Configuration

### Current Default: SD Card Enabled (Display Removed)

**Why?** SD card support is enabled by default, which requires removing the display due to pin conflicts.

#### Pin Conflicts
- **Display uses**: GPIO4 (SDA), GPIO5 (SCL)
- **SD card needs**: GPIO4 (MOSI), GPIO5 (SCK) ⚠️ **CONFLICTS!**

#### SD Card Setup
```
SD Card Module    ESP8266 D1 Mini
VCC        --->   3.3V
GND        --->   GND
MISO       --->   D6 (GPIO12)
MOSI       --->   D7 (GPIO13)
SCK        --->   D5 (GPIO14)
CS         --->   D8 (GPIO15)
```

#### Status Monitoring (No Display)
- **Serial Monitor**: Real-time status and debugging
- **STATS Command**: Send `STATS` via serial to check storage
- **Data Extraction**: Use `data_extractor.py` to get wardriving data

### Alternative: Display Enabled (SD Card Disabled)

If you prefer visual feedback over unlimited storage:

#### 1. Disable SD Card Support
```cpp
// In Wardriving.ino, change:
#define ENABLE_SD_CARD

// To:
// #define ENABLE_SD_CARD
```

#### 2. Display Setup
```
Display Module     ESP8266 D1 Mini
VCC         --->   3.3V
GND         --->   GND
SDA         --->   D2 (GPIO4)
SCL         --->   D1 (GPIO5)
```

#### 3. Storage Limitations
- **500KB Limit**: Internal flash storage only
- **Regular Extraction**: Extract data frequently
- **Visual Feedback**: Status shown on display

## 📊 Usage

### Basic Operation

1. **Power On**: Device initializes and searches for GPS signal
2. **GPS Lock**: Wait for GPS lock (shown on display or serial)
3. **Wardriving**: Device automatically scans WiFi and Bluetooth
4. **Data Collection**: All data stored in `/wardrive_data.csv`

### Display Information (when enabled)

The OLED shows real-time stats:
```
GPS: 8 sats          <- GPS satellite count
WiFi: 1,234          <- Total WiFi networks found
BT: 567              <- Total Bluetooth devices (ESP32 only)
Scans: 42            <- Number of scan cycles completed
33.123,-117.456      <- Current coordinates
```

### Storage Management

#### SD Card Configuration (Default)
- ✅ **Unlimited storage**: No storage limits
- ✅ **Data persists**: Across power cycles
- ❌ **No visual feedback**: Must use serial monitor

#### LittleFS Configuration (Display Mode)
- ⚠️ **500KB limit**: Internal flash storage only
- ⚠️ **400KB warning**: Display shows "LOW STORAGE"
- 🚨 **500KB full**: Stops logging, shows "STORAGE FULL!"

### Data Extraction

```bash
# Auto-detect device and extract data
make extract PORT=/dev/ttyUSB0

# Show statistics only
make stats PORT=/dev/ttyUSB0

# Save to specific file
make extract PORT=/dev/ttyUSB0 -o my_wardrive_data.csv

# Create WiGLE upload file
make wigle-format PORT=/dev/ttyUSB0

# Backup with timestamp
make backup PORT=/dev/ttyUSB0
```

### Sudo Commands (for permission issues)

```bash
# Extract data with sudo
make extract-sudo PORT=/dev/ttyUSB0

# Get statistics with sudo
make stats-sudo PORT=/dev/ttyUSB0

# Test connection with sudo
make test-connection-sudo PORT=/dev/ttyUSB0
```

## 📁 Data Format

The output CSV follows WiGLE standards:

```csv
MAC,SSID,AuthType,FirstSeen,Channel,RSSI,CurrentLatitude,CurrentLongitude,AltitudeMeters,AccuracyMeters,Type
AA:BB:CC:DD:EE:FF,MyNetwork,WPA2,2024-01-01 12:00:00,6,-45,33.12345678,-117.12345678,0,10,WIFI
11:22:33:44:55:66,SomeDevice,BT,2024-01-01 12:00:01,-1,-67,33.12345678,-117.12345678,0,10,BT
```

### Column Descriptions

- **MAC**: Device MAC address
- **SSID**: Network name (WiFi) or device name (Bluetooth)
- **AuthType**: Security/encryption type
- **FirstSeen**: Timestamp when first detected
- **Channel**: WiFi channel (-1 for Bluetooth)
- **RSSI**: Signal strength in dBm
- **CurrentLatitude/Longitude**: GPS coordinates
- **AltitudeMeters**: Elevation (set to 0)
- **AccuracyMeters**: GPS accuracy estimate
- **Type**: WIFI or BT

## 🔧 Advanced Configuration

### Custom Scan Intervals

```cpp
// In loop() function, change:
delay(5000); // 5 seconds
// To:
delay(10000); // 10 seconds for slower scanning
```

### Different Storage Options

- **SD Card**: Unlimited storage (default)
- **LittleFS**: 500KB limit (display mode)
- **Serial Streaming**: Send data directly over serial

### Additional Protocols

The framework can be extended to scan:
- **LoRa devices**
- **Zigbee networks**
- **Cell towers**

## 🛠️ Troubleshooting

### Upload Issues

#### Manual Flash Mode (ESP8266 D1 Mini)

If you get `Failed to connect to ESP8266: Timed out waiting for packet header`:

1. **Double-click the button** on the board quickly
2. **Immediately run** the upload command:
   ```bash
   make esp8266 PORT=/dev/ttyUSB0
   ```

#### Other Common Issues

- **Connection failed**: Check USB cable and port
- **Permission denied**: Use sudo commands or add user to dialout group
- **Port not found**: Try different USB port or cable
- **Compilation errors**: Run `make install-deps`

### GPS Issues

- **No GPS fix**: Ensure GPS module is connected and has clear sky view
- **Wrong coordinates**: Check GPS module wiring and antenna
- **Slow fix**: Wait 2-3 minutes for cold start

### WiFi Scanning Issues

- **No networks found**: Check WiFi antenna connection
- **Missing networks**: Some networks may be hidden or using non-standard channels

### Memory Issues

- **Device crashes**: Reduce scan frequency or implement data streaming
- **File full**: Extract data regularly or use SD card

### ESP8266 Specific

- **Memory limitations**: Consider reducing scan frequency or buffer sizes
- **No Bluetooth**: This is expected - ESP8266 doesn't support Bluetooth

### Display Issues

#### Black Screen

1. Check memory usage - should be <45% RAM
2. Verify pins 4,5 are connected to OLED (SCL, SDA)
3. Try power cycle (unplug/replug USB)

#### Memory Issues

- If RAM usage >50%, reduce `MAX_STORED_NETWORKS` in firmware
- Monitor compilation output for memory warnings

## 📈 Performance Tips

1. **Optimize scan timing**: Adjust delay between scans based on movement speed
2. **Regular data extraction**: Don't let storage fill up completely
3. **GPS accuracy**: Wait for good GPS lock before starting scans
4. **Power management**: Monitor battery level during long sessions

## 🗂️ Commands Reference

| Command | Description |
|---------|-------------|
| `make esp32` | Build and upload for ESP32 |
| `make esp8266` | Build and upload for ESP8266 |
| `make monitor` | Open serial monitor |
| `make extract` | Download data from device |
| `make stats` | Show quick statistics |
| `make backup` | Backup data with timestamp |
| `make wigle-format` | Create WiGLE upload file |
| `make extract-sudo` | Extract data with sudo |
| `make stats-sudo` | Get statistics with sudo |
| `make test-connection` | Test device connection |
| `make clean` | Clean build files |
| `make help` | Show all available commands |

## 🧹 Repository Maintenance

### Cleaning Up

```bash
# Clean build artifacts and temporary data
make clean

# What gets cleaned:
# - build/ directory
# - *.csv data files (personal location data)
# - Python cache files
# - Backup files
```

## 📤 Uploading to WiGLE

1. Extract data using `make extract` or `make wigle-format`
2. Visit [WiGLE.net](https://wigle.net)
3. Create account and go to "Upload"
4. Upload your CSV file
5. Wait for processing and view on map

## 🤝 Contributing

Feel free to submit improvements:

- Better power management
- Additional protocol support
- Enhanced data formats
- Performance optimizations

## 📄 License

Based on the original DNS DriveBy project. Use for educational and research purposes.

## 👨‍💻 Credits

- Original DNS DriveBy design by Alex Lynd

## 📞 Need Help?

Common issues and solutions:

1. **Device not detected**: Check USB cable and drivers
2. **GPS takes forever**: Go outside, wait patiently
3. **No Bluetooth on ESP8266**: This is normal, ESP8266 doesn't support BT
4. **Memory issues**: Extract data more frequently
5. **Compilation errors**: Run `make install-deps`
6. **SD card not detected**: Check wiring and format as FAT32
7. **Display not working**: Check pin conflicts with SD card

Happy wardriving! 🚗📡
