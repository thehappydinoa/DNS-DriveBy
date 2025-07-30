# DNS DriveBy Wardriving Firmware

This firmware repurposes your DNS DriveBy hardware for WiFi and Bluetooth wardriving, generating data compatible with WiGLE and other mapping platforms.

## Features

- **WiFi Scanning**: Detects all WiFi networks with MAC, SSID, encryption, signal strength, and location
- **Bluetooth Scanning**: Detects Bluetooth devices (ESP32 only)
- **GPS Integration**: Records precise coordinates for all detected devices
- **WiGLE Format**: Outputs data in standard WiGLE CSV format
- **Real-time Display**: Shows scanning progress and statistics on OLED display
- **SPIFFS Storage**: Stores data on device flash memory

## Hardware Compatibility

### ESP32 (Recommended)

- WiFi scanning ✅
- Bluetooth scanning ✅
- Better performance and memory

### ESP8266

- WiFi scanning ✅
- Bluetooth scanning ❌ (hardware limitation)

## Installation

### 1. Hardware Setup

Your DNS DriveBy device should already have:

- ESP32/ESP8266 microcontroller
- GPS module connected
- SH1106 OLED display
- Power supply (battery)

### 2. Firmware Upload

#### Option A: Arduino IDE

1. Install required libraries:

   ```
   - TinyGPS++
   - ESP32/ESP8266WiFi
   - BLE libraries (ESP32 only)
   - SH1106Wire
   ```

2. Select your board:
   - **ESP32**: ESP32 Dev Module
   - **ESP8266**: NodeMCU 1.0 or Wemos D1 Mini

3. Configure the firmware:
   - For ESP8266: Comment out `#define BOARD_ESP32`
   - For ESP32: Leave `#define BOARD_ESP32` uncommented

4. Upload `Wardriving.ino` to your device

#### Option B: PlatformIO

```bash
# Install PlatformIO
pip install platformio

# Configure for your board in platformio.ini
# Upload firmware
pio run --target upload
```

### 3. Configuration

Edit the following in `Wardriving.ino` if needed:

```cpp
// GPS Configuration
#define GPS_BAUD 9600

// Display pins (adjust for your wiring)
SH1106Wire display(0x3C, 4, 5); // I2C address, SDA, SCL

// Scan timing
delay(5000); // 5 second delay between scans
```

## Usage

### Basic Operation

1. **Power On**: The device will initialize and search for GPS signal
2. **GPS Lock**: Wait for GPS lock (shown on display)
3. **Wardriving**: Device automatically scans WiFi and Bluetooth
4. **Data Collection**: All data stored in `/wardrive_data.csv` on device

### Display Information

The OLED shows real-time stats:

```
GPS: 8 sats          <- GPS satellite count
WiFi: 1,234          <- Total WiFi networks found
BT: 567              <- Total Bluetooth devices (ESP32 only)
Scans: 42            <- Number of scan cycles completed
33.123,-117.456      <- Current coordinates
```

### Data Extraction

Use the included Python tool to extract data:

```bash
# Auto-detect device and extract data
python3 data_extractor.py

# Specify port manually
python3 data_extractor.py -p /dev/ttyUSB0

# Show statistics only
python3 data_extractor.py -s

# Save to specific file
python3 data_extractor.py -o my_wardrive_data.csv
```

## Data Format

The output CSV follows WiGLE standards:

```csv
WigleWifi-1.4,appRelease=1.0,model=DNS-DriveBy-Wardrive,release=1.0,device=ESP32-Wardriver,display=SH1106,board=ESP32,brand=Custom
MAC,SSID,AuthMode,FirstSeen,Channel,RSSI,CurrentLatitude,CurrentLongitude,AltitudeMeters,AccuracyMeters,Type
AA:BB:CC:DD:EE:FF,MyNetwork,[WPA2-PSK-CCMP+TKIP][ESS],2024-01-01 12:00:00,6,-45,33.12345678,-117.12345678,0,10,WIFI
11:22:33:44:55:66,SomeDevice,[BT],2024-01-01 12:00:01,-1,-67,33.12345678,-117.12345678,0,10,BT
```

### Column Descriptions

- **MAC**: Device MAC address
- **SSID**: Network name (for WiFi) or device name (for Bluetooth)
- **AuthMode**: Security/encryption type
- **FirstSeen**: Timestamp when first detected
- **Channel**: WiFi channel (-1 for Bluetooth)
- **RSSI**: Signal strength in dBm
- **CurrentLatitude/Longitude**: GPS coordinates
- **AltitudeMeters**: Elevation (set to 0)
- **AccuracyMeters**: GPS accuracy estimate
- **Type**: WIFI or BT

## Uploading to WiGLE

1. Extract data using `data_extractor.py`
2. Visit [WiGLE.net](https://wigle.net)
3. Create account and go to "Upload"
4. Upload your CSV file
5. Wait for processing and view on map

## Troubleshooting

### GPS Issues

- **No GPS fix**: Ensure GPS module is connected and has clear sky view
- **Wrong coordinates**: Check GPS module wiring and antenna

### WiFi Scanning Issues

- **No networks found**: Check WiFi antenna connection
- **Missing networks**: Some networks may be hidden or using non-standard channels

### Memory Issues

- **Device crashes**: Reduce scan frequency or implement data streaming
- **File full**: Extract data regularly or increase SPIFFS partition size

### ESP8266 Specific

- **Memory limitations**: Consider reducing scan frequency or buffer sizes
- **No Bluetooth**: This is expected - ESP8266 doesn't support Bluetooth

## Performance Tips

1. **Optimize scan timing**: Adjust delay between scans based on movement speed
2. **Regular data extraction**: Don't let SPIFFS fill up completely
3. **GPS accuracy**: Wait for good GPS lock before starting scans
4. **Power management**: Monitor battery level during long sessions

## Advanced Configuration

### Custom Scan Intervals

```cpp
// In loop() function, change:
delay(5000); // 5 seconds
// To:
delay(10000); // 10 seconds for slower scanning
```

### Different Storage Options

- **SD Card**: Modify code to use SD card instead of SPIFFS
- **Serial Streaming**: Send data directly over serial instead of storing

### Additional Protocols

The framework can be extended to scan other protocols:

- **LoRa devices**
- **Zigbee networks**
- **Cell towers**

## Contributing

Feel free to submit improvements:

- Better power management
- Additional protocol support
- Enhanced data formats
- Performance optimizations

## 🔧 Troubleshooting

### Upload Issues

#### Manual Flash Mode (ESP8266 D1 Mini)

If you get `Failed to connect to ESP8266: Timed out waiting for packet header`:

1. **Double-click the button** on the board quickly
2. **Immediately run** the upload command:

   ```bash
   arduino-cli upload -b esp8266:esp8266:d1_mini --port /dev/cu.usbserial-XXX .
   ```

3. The double-click puts the ESP8266 into flash mode manually

#### Other Common Issues

- **Connection failed**: Check USB cable and port
- **Permission denied**: Add user to dialout group (Linux)  
- **Port not found**: Try different USB port or cable
- **Compilation errors**: Ensure all libraries are installed
- **Display not working**: Check memory usage in compilation output
- **No serial output**: Verify baud rate is 115200

### Display Issues

#### Black Screen

1. Check memory usage - should be <45% RAM
2. Verify pins 4,5 are connected to OLED (SCL, SDA)
3. Try power cycle (unplug/replug USB)

#### Memory Issues

- If RAM usage >50%, reduce `MAX_STORED_NETWORKS` in firmware
- Monitor compilation output for memory warnings

## License

Based on the original DNS DriveBy project. Use for educational and research purposes.

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

## Credits

- Original DNS DriveBy design by Alex Lynd
