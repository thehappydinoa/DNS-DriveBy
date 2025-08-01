//#define BOARD_ESP32 // Comment this out if using ESP8266

// Firmware Version Information
#define FIRMWARE_VERSION "2.2.0-5a3f058"
#define FIRMWARE_NAME "DNS-DriveBy Wardriving"
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <SH1106Wire.h>
#include <Wire.h>
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>

// Hardware Configuration - CORRECT PINS DISCOVERED BY DIAGNOSTIC!
#define GPSRX D4
#define GPSTX D3
SoftwareSerial GPS_SERIAL(GPSRX, GPSTX);
TinyGPSPlus gps;

// Display with WORKING pin configuration: GPIO4=SDA, GPIO5=SCL
SH1106Wire display(0x3C, 4, 5);  // Correct pins!

// Data Storage
String csvFilename = "/wardrive_data.csv";

// Stats tracking
struct WardrivingStats {
  uint32_t wifiNetworks = 0;
  uint32_t totalScans = 0;
  float currentLat = 0.0;
  float currentLng = 0.0;
  uint8_t satellites = 0;
  bool gpsActive = false;
  // GPS time tracking
  uint16_t currentYear = 0;
  uint8_t currentMonth = 0;
  uint8_t currentDay = 0;
  uint8_t currentHour = 0;
  uint8_t currentMinute = 0;
  uint8_t currentSecond = 0;
  bool timeValid = false;
};

WardrivingStats stats;

// Storage management
#define MAX_DATA_FILE_SIZE 500000    // 500KB max file size (LittleFS limit)
#define STORAGE_WARNING_THRESHOLD 400000  // Warn at 400KB
#define MIN_FREE_SPACE 50000         // Stop logging at 50KB free space
bool storageFull = false;
bool storageWarningShown = false;
unsigned long lastStorageCheck = 0;

// GPS retry mechanism
unsigned long lastGPSRetry = 0;
unsigned long lastDisplayUpdate = 0;
bool displayWorking = true;

String getWiFiEncryption(uint8_t encryptionType) {
  switch (encryptionType) {
    case ENC_TYPE_WEP: return "WEP";
    case ENC_TYPE_TKIP: return "WPA";
    case ENC_TYPE_CCMP: return "WPA2";
    case ENC_TYPE_NONE: return "Open";
    case ENC_TYPE_AUTO: return "Auto";
    default: return "Unknown";
  }
}

String getCurrentTimestamp() {
  static char timestamp[20];
  
  if (stats.timeValid) {
    // Use real GPS time (UTC)
    sprintf(timestamp, "%04d-%02d-%02d %02d:%02d:%02d",
            stats.currentYear, stats.currentMonth, stats.currentDay,
            stats.currentHour, stats.currentMinute, stats.currentSecond);
  } else {
    // Fallback: use relative timestamp based on uptime
    unsigned long totalSeconds = millis() / 1000;
    unsigned long hours = totalSeconds / 3600;
    unsigned long minutes = (totalSeconds % 3600) / 60;
    unsigned long seconds = totalSeconds % 60;
    
    sprintf(timestamp, "2025-01-01 %02lu:%02lu:%02lu", hours % 24, minutes, seconds);
  }
  
  return String(timestamp);
}

void checkStorageSpace() {
  // Check storage every 30 seconds
  if (millis() - lastStorageCheck < 30000) return;
  lastStorageCheck = millis();
  
  FSInfo fs_info;
  LittleFS.info(fs_info);
  
  size_t freeSpace = fs_info.totalBytes - fs_info.usedBytes;
  
  // Check if CSV file exists and get its size
  File dataFile = LittleFS.open(csvFilename, "r");
  size_t fileSize = 0;
  if (dataFile) {
    fileSize = dataFile.size();
    dataFile.close();
  }
  
  Serial.printf("[STORAGE] Free: %d bytes, CSV size: %d bytes\n", freeSpace, fileSize);
  
  // Check for storage full condition
  if (freeSpace < MIN_FREE_SPACE || fileSize > MAX_DATA_FILE_SIZE) {
    if (!storageFull) {
      storageFull = true;
      Serial.println("[WARNING] Storage limit reached - stopping data logging!");
      
      display.clear();
      display.drawString(0, 0, "STORAGE FULL!");
      display.drawString(0, 12, "Data logging");
      display.drawString(0, 24, "stopped");
      display.drawString(0, 36, "Extract data:");
      display.drawString(0, 48, "READ_FILE cmd");
      display.display();
      delay(5000);
    }
  }
  // Check for warning threshold
  else if ((freeSpace < (MIN_FREE_SPACE * 2) || fileSize > STORAGE_WARNING_THRESHOLD) && !storageWarningShown) {
    storageWarningShown = true;
    Serial.println("[WARNING] Storage getting full - extract data soon!");
    
    display.clear();
    display.drawString(0, 0, "LOW STORAGE!");
    display.drawString(0, 12, "Extract data");
    display.drawString(0, 24, "soon");
    display.drawString(0, 36, String(freeSpace/1000) + "KB free");
    display.display();
    delay(3000);
  }
}

void showStorageWarning() {
  display.clear();
  display.drawString(0, 0, "WARNING:");
  display.drawString(0, 12, "No SD card!");
  display.drawString(0, 24, "Using internal");
  display.drawString(0, 36, "flash (~500KB)");
  display.drawString(0, 48, "Limited space!");
  display.display();
  
  Serial.println("[WARNING] No SD card detected - using internal flash storage");
  Serial.println("[WARNING] Internal storage limited to ~500KB total");
  Serial.println("[WARNING] Extract data regularly to prevent storage full");
}

void initializeDisplay() {
  Serial.println("[+] Initializing OLED display...");
  
  // Initialize I2C with WORKING pins: GPIO4=SDA, GPIO5=SCL
  Wire.begin(4, 5);
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  
  // Simple startup message
  display.clear();
  display.drawString(0, 0, FIRMWARE_NAME);
  display.drawString(0, 12, "v" FIRMWARE_VERSION);
  display.drawString(0, 24, "ESP8266 D1 Mini");
  display.drawString(0, 36, "GPIO4/5 I2C");
  display.display();
  
  Serial.println("[+] OLED display ready!");
  delay(2000);
}

bool initializeFileSystem() {
  Serial.println("[+] Initializing file system...");
  
  if (!LittleFS.begin()) {
    Serial.println("[-] LittleFS mount failed");
    return false;
  }
  
  Serial.println("[+] File system ready");
  return true;
}

void quickDisplayUpdate() {
  // Only update display every 10 seconds to reduce memory pressure
  if (millis() - lastDisplayUpdate < 10000) return;
  
  display.clear();
  
  // Show storage status if there are issues
  if (storageFull) {
    display.drawString(0, 0, "STORAGE FULL!");
    display.drawString(0, 12, "WiFi: " + String(stats.wifiNetworks));
    display.drawString(0, 24, "Scans: " + String(stats.totalScans));
    display.drawString(0, 36, "Extract data");
    display.drawString(0, 48, "via serial");
  } else if (storageWarningShown) {
    display.drawString(0, 0, "LOW STORAGE");
    display.drawString(0, 12, "WiFi: " + String(stats.wifiNetworks));
    display.drawString(0, 24, "Scans: " + String(stats.totalScans));
    display.drawString(0, 36, "Extract soon");
    display.drawString(0, 48, "LOGGING...");
  } else {
    // Normal display
    display.drawString(0, 0, "Wardriving");
    display.drawString(0, 12, "WiFi: " + String(stats.wifiNetworks));
    display.drawString(0, 24, "Scans: " + String(stats.totalScans));
    
    // Show GPS and time status
    if (stats.gpsActive && stats.timeValid) {
      display.drawString(0, 36, "GPS: " + String(stats.satellites) + " T:OK");
    } else if (stats.gpsActive) {
      display.drawString(0, 36, "GPS: " + String(stats.satellites) + " T:--");
    } else {
      display.drawString(0, 36, "GPS: No fix");
    }
    
    display.drawString(0, 48, "WORKING!");
  }
  
  display.display();
  lastDisplayUpdate = millis();
}

void updateGPS() {
  while (GPS_SERIAL.available()) {
    if (gps.encode(GPS_SERIAL.read())) {
      // Update location if valid
      if (gps.location.isValid()) {
        stats.currentLat = gps.location.lat();
        stats.currentLng = gps.location.lng();
        stats.satellites = gps.satellites.value();
        if (!stats.gpsActive) {
          stats.gpsActive = true;
          Serial.printf("[+] GPS Fix: %.6f, %.6f (Sats: %d)\n", 
                       stats.currentLat, stats.currentLng, stats.satellites);
        }
      }
      
      // Update time if valid
      if (gps.date.isValid() && gps.time.isValid()) {
        stats.currentYear = gps.date.year();
        stats.currentMonth = gps.date.month();
        stats.currentDay = gps.date.day();
        stats.currentHour = gps.time.hour();
        stats.currentMinute = gps.time.minute();
        stats.currentSecond = gps.time.second();
        
        if (!stats.timeValid) {
          stats.timeValid = true;
          Serial.printf("[+] GPS Time: %04d-%02d-%02d %02d:%02d:%02d UTC\n",
                       stats.currentYear, stats.currentMonth, stats.currentDay,
                       stats.currentHour, stats.currentMinute, stats.currentSecond);
        }
      }
    }
  }
  
  // GPS retry every 2 minutes
  if (!stats.gpsActive && (millis() - lastGPSRetry > 120000)) {
    Serial.println("[+] Retrying GPS...");
    lastGPSRetry = millis();
  }
}

void scanWiFi() {
  int networksFound = WiFi.scanNetworks();
  
  if (networksFound > 0) {
    Serial.printf("[ESP] Found %d networks:\n", networksFound);
    
    int loggedCount = 0;
    File dataFile;
    
    // Only open file for writing if storage isn't full
    if (!storageFull) {
      dataFile = LittleFS.open(csvFilename, "a");
    }
    
    for (int i = 0; i < networksFound; i++) {
      String ssid = WiFi.SSID(i);
      String bssid = WiFi.BSSIDstr(i);
      int rssi = WiFi.RSSI(i);
      uint8_t encType = WiFi.encryptionType(i);
      String encryption = getWiFiEncryption(encType);
      
      // Always log to serial
      if (storageFull) {
        Serial.printf("[ESP] [%d] %s (%s) - %ddBm [NOT LOGGED - STORAGE FULL]\n", 
                     i+1, ssid.c_str(), bssid.c_str(), rssi);
      } else {
        Serial.printf("[ESP] [%d] %s (%s) - %ddBm\n", 
                     i+1, ssid.c_str(), bssid.c_str(), rssi);
      }
      
      // Write to file only if storage available
      if (dataFile && !storageFull) {
        // WiGLE CSV format with real timestamp
        String timestamp = getCurrentTimestamp();
        dataFile.printf("%s,%s,%s,%s,%d,%d,%.8f,%.8f,0,0,WIFI\n",
                        bssid.c_str(),
                        ssid.c_str(),
                        encryption.c_str(),
                        timestamp.c_str(),
                        WiFi.channel(i),
                        rssi,
                        stats.currentLat,
                        stats.currentLng);
        stats.wifiNetworks++;
        loggedCount++;
      }
    }
    
    if (dataFile) dataFile.close();
    
    if (storageFull) {
      Serial.printf("[ESP] Summary: %d networks found, 0 logged (STORAGE FULL)\n", networksFound);
    } else {
      Serial.printf("[ESP] Summary: %d networks found, %d logged\n", networksFound, loggedCount);
    }
  }
  
  WiFi.scanDelete();
  stats.totalScans++;
}

void handleSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command.startsWith("READ_FILE:")) {
      String filename = command.substring(10);
      Serial.println("FILE_START");
      
      File file = LittleFS.open(filename, "r");
      if (file) {
        while (file.available()) {
          Serial.println(file.readStringUntil('\n'));
        }
        file.close();
      }
      Serial.println("FILE_END");
      
    } else if (command == "STATS") {
      Serial.println("STATS_START");
      Serial.println("WiFi Networks: " + String(stats.wifiNetworks));
      Serial.println("Total Scans: " + String(stats.totalScans));
      Serial.println("GPS Active: " + String(stats.gpsActive ? "true" : "false"));
      Serial.println("GPS Time Valid: " + String(stats.timeValid ? "true" : "false"));
      
      if (stats.timeValid) {
        String currentTime = getCurrentTimestamp();
        Serial.println("Current Time: " + currentTime + " UTC");
      }
      
      // Add storage info
      FSInfo fs_info;
      LittleFS.info(fs_info);
      size_t freeSpace = fs_info.totalBytes - fs_info.usedBytes;
      File dataFile = LittleFS.open(csvFilename, "r");
      size_t fileSize = dataFile ? dataFile.size() : 0;
      if (dataFile) dataFile.close();
      
      Serial.println("Storage Free: " + String(freeSpace) + " bytes");
      Serial.println("CSV File Size: " + String(fileSize) + " bytes");
      Serial.println("Storage Full: " + String(storageFull ? "true" : "false"));
      Serial.println("STATS_END");
      
    } else if (command == "DELETE_DATA") {
      LittleFS.remove(csvFilename);
      stats.wifiNetworks = 0;
      stats.totalScans = 0;
      Serial.println("DATA_DELETED");
      
    } else if (command == "DISPLAY_RESET") {
      Serial.println("Resetting display...");
      initializeDisplay();
      Serial.println("Display reset complete");
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n*********************************");
  Serial.println("   " FIRMWARE_NAME);
  Serial.println("   Version: " FIRMWARE_VERSION);
  Serial.println("   Build: " BUILD_DATE " " BUILD_TIME);
  Serial.println("   Hardware: ESP8266 D1 Mini");
  Serial.println("*********************************");
  
  // Initialize display with correct pins
  initializeDisplay();
  
  // Initialize file system
  if (!initializeFileSystem()) {
    Serial.println("[-] FATAL: Storage initialization failed");
    return;
  }
  
  // Show storage warning (no SD card on ESP8266 D1 Mini)
  showStorageWarning();
  delay(3000);
  
  // Initialize WiFi
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  // Initialize GPS (quick check)
  GPS_SERIAL.begin(9600);
  Serial.println("[+] GPS initialized (background mode)");
  lastGPSRetry = millis();
  
  // Write CSV header if file doesn't exist
  if (!LittleFS.exists(csvFilename)) {
    File dataFile = LittleFS.open(csvFilename, "w");
    if (dataFile) {
      dataFile.println("MAC,SSID,AuthType,FirstSeen,Channel,RSSI,CurrentLatitude,CurrentLongitude,AltitudeMeters,AccuracyMeters,Type");
      dataFile.close();
      Serial.println("[+] Created CSV file with header");
    }
  }
  
  Serial.println("[+] Wardriving ready!");
  Serial.println("[+] Scanning every 5 seconds...");
  Serial.println("[+] Display updates every 10 seconds (memory optimized)");
  Serial.println("[+] Storage limit: 500KB - extract data regularly");
  Serial.println("[+] Send 'STATS' to check storage, 'DISPLAY_RESET' to reset display");
}

void loop() {
  // Update GPS data
  updateGPS();
  
  // Handle serial commands
  handleSerialCommands();
  
  // Check storage space periodically
  checkStorageSpace();
  
  // Perform scans every 5 seconds
  static unsigned long lastScan = 0;
  if (millis() - lastScan > 5000) {
    Serial.println("\n--- Starting scan cycle ---");
    
    scanWiFi();
    
    // Only update display occasionally to reduce memory pressure
    quickDisplayUpdate();
    
    lastScan = millis();
    
    if (storageFull) {
      Serial.printf("--- Scan complete (WiFi: %d, Scans: %d) [STORAGE FULL] ---\n", 
                    stats.wifiNetworks, stats.totalScans);
    } else {
      Serial.printf("--- Scan complete (WiFi: %d, Scans: %d) ---\n", 
                    stats.wifiNetworks, stats.totalScans);
    }
  }
  
  // Handle serial commands (STATS, READ_FILE, etc.)
  handleSerialCommands();
  
  delay(100);
} 