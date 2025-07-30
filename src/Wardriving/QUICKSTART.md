# Quick Start Guide - DNS DriveBy Wardriving

Get your DNS DriveBy device wardriving in 15 minutes!

## Step 1: Hardware Check

Make sure your DNS DriveBy device has:

- ✅ ESP32 or ESP8266 microcontroller
- ✅ GPS module (working and getting signal)
- ✅ SH1106 OLED display
- ✅ Battery/power source
- ✅ USB cable for programming

## Step 2: Software Setup

### Install Arduino CLI (Recommended)

```bash
# macOS
brew install arduino-cli

# Linux
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

# Windows - Download from: https://arduino.github.io/arduino-cli/
```

### Install Dependencies

```bash
cd src/Wardriving
make install-deps
```

## Step 3: Configure for Your Hardware

Edit `Wardriving.ino`:

**For ESP32 (recommended):**

```cpp
#define BOARD_ESP32  // Keep this line uncommented
```

**For ESP8266:**

```cpp
//#define BOARD_ESP32  // Comment out this line
```

## Step 4: Upload Firmware

Connect your device via USB and run:

```bash
# For ESP32
make esp32

# For ESP8266  
make esp8266

# Or specify port manually
make esp32 PORT=/dev/ttyUSB0
```

### 🚨 Upload Troubleshooting

If you get **"Failed to connect to ESP8266: Timed out waiting for packet header"**:

1. **Double-click the button** on your board quickly
2. **Immediately run** the upload command again:

   ```bash
   make esp8266  # or make esp32
   ```

## Step 5: Test Your Device

### Monitor Serial Output

```bash
make monitor
```

You should see:

```
*********************************
   WiFi/Bluetooth Wardriving
   Based on DNS DriveBy Hardware
*********************************
[+] GPS initialized
[+] GPS Fix: 33.123456, -117.123456 (Sats: 8)
[+] Wardriving ready!
```

### Check the Display

Your OLED should show:

```
GPS: 8 sats
WiFi: 0
BT: 0
Scans: 0
33.123,-117.123
```

## Step 6: Start Wardriving

1. **Power on** your device
2. **Wait for GPS lock** (may take 1-2 minutes outdoors)
3. **Start moving** - device scans automatically every 5 seconds
4. **Watch the counters** increase on the display

## Step 7: Extract Your Data

### Get Statistics

```bash
make stats
```

### Download All Data

```bash
make extract
```

### Create WiGLE Upload File

```bash
make wigle-format
```

## Example Session

Here's what a typical wardriving session looks like:

```bash
# 1. Upload firmware
make esp32

# 2. Monitor device startup
make monitor
# Wait for "GPS Fix" message, then Ctrl+C to exit

# 3. Go wardriving for 30 minutes

# 4. Check results
make stats
# Output:
# ========================================
#          WARDRIVING STATISTICS
# ========================================
# Total Entries:      1,234
# WiFi Networks:      1,100
# Bluetooth Devices:  134
# Unique Locations:   45
# ========================================

# 5. Extract data for WiGLE
make wigle-format
# Creates: wigle_upload_20240101_120000.csv

# 6. Upload to WiGLE.net
# Visit https://wigle.net/uploads
```

## Troubleshooting

### ❌ GPS Not Getting Fix

- Go outside with clear sky view
- Check GPS wiring (RX/TX pins)
- Wait 2-3 minutes for cold start

### ❌ No Networks Found

- Check WiFi antenna connection
- Verify device is moving through areas with WiFi
- Check serial output for scan messages

### ❌ Upload Fails

```bash
# Check port
make device-info

# Test connection
make test-connection PORT=/dev/ttyUSB0

# Try different port
make esp32 PORT=/dev/ttyACM0
```

### ❌ Compilation Errors

```bash
# Install dependencies
make install-deps

# Clean and retry
make clean
make esp32
```

## Data Formats

Your wardriving data can be converted to multiple formats:

```bash
# Extract raw WiGLE CSV
python3 data_extractor.py -o my_data.csv

# Convert to other formats
python3 data_converter.py my_data.csv -f geojson -o map.geojson
python3 data_converter.py my_data.csv -f kml -o google_earth.kml
python3 data_converter.py my_data.csv -f kismet -o kismet_data.csv
```

## Performance Tips

### Optimize for Battery Life

```cpp
// In Wardriving.ino, change scan interval:
delay(10000); // 10 seconds instead of 5
```

### Optimize for Dense Areas

```cpp
// Faster scanning for urban areas:
delay(2000); // 2 seconds between scans
```

### Storage Management

```bash
# Regular backups during long sessions
make backup

# Clear device storage
# Send "DELETE_DATA" command via serial monitor
```

## What's Next?

### Upload to WiGLE

1. Go to [WiGLE.net](https://wigle.net)
2. Create account
3. Go to "Upload" section
4. Upload your CSV file
5. View your data on the world map!

### Analyze Your Data

```bash
# Get detailed statistics
python3 data_converter.py my_data.csv -s

# Create map visualization
python3 data_converter.py my_data.csv -f geojson -o my_map.geojson
# Upload to: https://geojson.io
```

### Advanced Features

- Connect external antenna for better range
- Add SD card for more storage
- Modify code for different scan protocols
- Add LoRa/Zigbee scanning

## Commands Reference

| Command | Description |
|---------|-------------|
| `make esp32` | Build and upload for ESP32 |
| `make esp8266` | Build and upload for ESP8266 |
| `make monitor` | Open serial monitor |
| `make extract` | Download data from device |
| `make stats` | Show quick statistics |
| `make backup` | Backup data with timestamp |
| `make wigle-format` | Create WiGLE upload file |
| `make clean` | Clean build files |

## Need Help?

Common issues and solutions:

1. **Device not detected**: Check USB cable and drivers
2. **GPS takes forever**: Go outside, wait patiently
3. **No Bluetooth on ESP8266**: This is normal, ESP8266 doesn't support BT
4. **Memory issues**: Extract data more frequently
5. **Compilation errors**: Run `make install-deps`

Happy wardriving! 🚗📡
