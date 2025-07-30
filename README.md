<p align="center">
<img src="images/DNS-DriveBy_Logo.svg" height=150px>
</p>

# DNS DriveBy - Multi-Purpose ESP8266 Platform

## 📡 **Wardriving Firmware** (New!)

**Transform your DNS DriveBy into a powerful wardriving device!** The wardriving firmware repurposes the hardware for WiFi and Bluetooth reconnaissance with WiGLE-compatible data logging.

### ✨ **Wardriving Features:**

- 📍 **GPS Integration** - Records precise coordinates with network data
- 📱 **OLED Display** - Live statistics and status updates  
- 🌐 **WiFi Scanning** - Captures all nearby networks every 5 seconds
- 💾 **WiGLE Format** - Direct compatibility with community mapping
- 🔧 **Memory Optimized** - Runs efficiently on ESP8266 hardware
- 📊 **Data Extraction** - Python tools for easy data retrieval

**→ [Quick Start Guide](src/Wardriving/QUICKSTART.md) | [Full Documentation](src/Wardriving/README.md)**

---

## 🔍 **Original DNS DriveBy**

DNS-Driveby is a $10 tracker that uses Open Wi-Fi networks for telemetry & reconnaissance, instead of a SIM card. It logs GPS coordinates & other data to its internal memory, scans for un-authenticated Wi-Fi networks, and uses DNS Exfiltration through [DNS CanaryTokens](https://canarytokens.org) to bypass captive portals! Read about how Alex Lynd created it on [Hackster](https://www.hackster.io/alexlynd/dns-driveby-stealthy-gps-tracking-using-open-wi-fi-65730a)!

---

## 🚀 **Quick Setup**

### For Wardriving

```bash
cd src/Wardriving
make esp8266          # Upload wardriving firmware
make monitor           # View live data collection
```

### For Original DNS Tracking

```bash
cd src/Demo
# Follow original build instructions
```

## 📁 **Project Structure**

```bash
├── src/
│   ├── Wardriving/    # 🆕 Wardriving firmware & tools
│   │   ├── Wardriving.ino         # ESP8266 firmware
│   │   ├── data_extractor.py      # Extract data from device
│   │   ├── QUICKSTART.md          # 15-minute setup guide
│   │   └── README.md              # Full documentation
│   └── Demo/          # Original DNS DriveBy firmware
├── tests/             # Test utilities and data
└── images/            # Logos and documentation images
```

## 📚 **Resources**

### Original DNS DriveBy

- Stealthy GPS Tracking Using Open Wi-Fi - [Hackster](https://www.hackster.io/alexlynd/dns-driveby-stealthy-gps-tracking-using-open-wi-fi-65730a)
- Using DNS DriveBy as a GPS Tracker - [Hak5](https://youtu.be/H0Nwff0KDJ0?t=151)
- [Assembly Guide](https://dnsdriveby.com/guides/get-started/assembly/)

### Wardriving Resources

- [WiGLE.net](https://wigle.net) - Community WiFi mapping
- [Wardriving Guide](src/Wardriving/README.md) - Complete documentation

## 📝 **Changelog**

### v2.0 - Wardriving Edition ✨

- **NEW**: Complete wardriving firmware for WiFi reconnaissance
- **NEW**: WiGLE-compatible data logging and extraction
- **NEW**: Live OLED display with WiFi/GPS statistics  
- **NEW**: Python data extraction and conversion tools
- **NEW**: 15-minute quick start guide and comprehensive docs
- **FIXED**: Display compatibility issues resolved
- **FIXED**: Memory optimization for stable operation

### v1.2 - Original DNS DriveBy

- MicroSD Card Support
- JST Switch connector  
- Route LiPo under board
- SH1106 / SSD1306 Add-On

## 🔮 **Future Plans**

### Wardriving Enhancements

- Bluetooth scanning for ESP32 boards
- On-device deduplication optimization
- Real-time GPS timestamps
- Web interface for data viewing

### Original DNS DriveBy

- Fix GPS Traces
- Larger Resistor Footprint  
- Improved BMS
- OTA Update via Web Interface

## 📝 **Documented Issues** (Note to Self / Future Docs)

- Arduino-CLI VSCode Plugin doesn't support SPIFFS build

## ⚖️ **License & Ethics**

This project is intended for **educational and research purposes only**. When wardriving:

- Follow local laws and regulations
- Respect privacy and private property
- Use data responsibly for legitimate security research
- Consider contributing anonymized data to community projects like WiGLE

Based on the original DNS DriveBy project by Alex Lynd.
