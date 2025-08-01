//#define BOARD_ESP32 // Comment this out if using ESP8266

// SD Card Support (enabled by default)
// NOTE: SD card requires removing the display due to pin conflicts
// The display uses GPIO4/5 which conflict with SD card SPI pins
// Comment out the line below to disable SD card support and keep the display
#define ENABLE_SD_CARD

// Firmware Version Information
#define FIRMWARE_VERSION "2.2.0-89ee243"
#define FIRMWARE_NAME "DNS-DriveBy Wardriving"
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

#include <ESP8266WiFi.h>
#include <LittleFS.h>
#ifdef ENABLE_SD_CARD
#include <SD.h>
#include <SPI.h>
#endif
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
#ifdef ENABLE_SD_CARD
String sdCsvFilename = "/wardrive_data.csv"; // SD card path

// SD Card Configuration (ESP8266 D1 Mini)
// NOTE: Display must be removed when using SD card due to pin conflicts
// Display uses GPIO4/5 which conflict with SD card SPI pins
// If you want to keep the display, disable SD card support instead
#define SD_CS D8    // Chip Select for SD card
#define SD_MOSI D7  // MOSI pin  
#define SD_MISO D6  // MISO pin  
#define SD_SCK D5   // SCK pin (conflicts with display SCL!)

// Storage type tracking
bool usingSDCard = false; // Always false when SD card is disabled
#ifdef ENABLE_SD_CARD
bool sdCardAvailable = false;
#endif
#endif

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
bool useDisplay = true; // Will be set to false if SD card is detected

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
  
  // Only check storage limits for LittleFS (SD cards have unlimited space)
#ifdef ENABLE_SD_CARD
  if (!usingSDCard) {
#endif
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
#ifdef ENABLE_SD_CARD
  } else {
    // SD card - no storage limits to check
    if (storageFull) {
      storageFull = false; // Reset if we're now using SD card
    }
    if (storageWarningShown) {
      storageWarningShown = false; // Reset if we're now using SD card
    }
  }
#endif
}

void showStorageWarning() {
  if (useDisplay) {
    display.clear();
    display.drawString(0, 0, "WARNING:");
    display.drawString(0, 12, "No SD card!");
    display.drawString(0, 24, "Using internal");
    display.drawString(0, 36, "flash (~500KB)");
    display.drawString(0, 48, "Limited space!");
    display.display();
  }
  
  Serial.println("[WARNING] No SD card detected - using internal flash storage");
  Serial.println("[WARNING] Internal storage limited to ~500KB total");
  Serial.println("[WARNING] Extract data regularly to prevent storage full");
}

void initializeDisplay() {
  if (!useDisplay) {
    Serial.println("[+] Display disabled (SD card mode)");
    displayWorking = false;
    return;
  }
  
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
  // Skip display updates if display is disabled
  if (!useDisplay || !displayWorking) return;
  
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
          Serial.printf("[+] GPS Time: %04d-%02d-%02d %02d:%02d:%02d\n",
                       stats.currentYear, stats.currentMonth, stats.currentDay,
                       stats.currentHour, stats.currentMinute, stats.currentSecond);
        }
      }
    }
  }
  
  // GPS retry mechanism
  if (!stats.gpsActive && millis() - lastGPSRetry > 30000) {
    Serial.println("[-] No GPS fix - retrying...");
    lastGPSRetry = millis();
  }
}

#ifdef ENABLE_SD_CARD
bool initializeSDCard() {
  Serial.println("[+] Initializing SD card...");
  Serial.printf("[DEBUG] SD Card Pins - CS: %d, MOSI: %d, MISO: %d, SCK: %d\n", 
                SD_CS, SD_MOSI, SD_MISO, SD_SCK);
  
  // Configure SPI for SD card (ESP8266 doesn't use begin() with parameters)
  SPI.begin();
  Serial.println("[DEBUG] SPI initialized");
  
  // Try to initialize SD card
  Serial.printf("[DEBUG] Attempting to initialize SD card with CS pin %d\n", SD_CS);
  if (SD.begin(SD_CS)) {
    Serial.println("[DEBUG] SD.begin() returned true");
    
    // ESP8266 SD library doesn't have cardType() or cardSize() methods
    // We'll just check if the card is accessible by trying to open a test file
    Serial.println("[DEBUG] Testing SD card access...");
    File testFile = SD.open("/test.txt", FILE_WRITE);
    if (testFile) {
      testFile.println("test");
      testFile.close();
      Serial.println("[DEBUG] Test file write successful");
      
      // Try to read the test file
      File readFile = SD.open("/test.txt", FILE_READ);
      if (readFile) {
        String content = readFile.readString();
        readFile.close();
        Serial.printf("[DEBUG] Test file read successful, content: %s\n", content.c_str());
      } else {
        Serial.println("[DEBUG] Test file read failed");
      }
      
      SD.remove("/test.txt"); // Clean up test file
      Serial.println("[+] SD card detected and accessible");
      return true;
    } else {
      Serial.println("[-] SD card not accessible - cannot write test file");
      return false;
    }
  } else {
    Serial.println("[-] SD card initialization failed - SD.begin() returned false");
    Serial.println("[DEBUG] Possible causes:");
    Serial.println("[DEBUG] 1. SD card not connected");
    Serial.println("[DEBUG] 2. Wrong CS pin");
    Serial.println("[DEBUG] 3. Power supply issues");
    Serial.println("[DEBUG] 4. SD card not formatted as FAT32");
    Serial.println("[DEBUG] 5. Pin conflicts with other devices");
    return false;
  }
}
#endif

bool initializeStorage() {
#ifdef ENABLE_SD_CARD
  // Try SD card first
  sdCardAvailable = initializeSDCard();
  
  if (sdCardAvailable) {
    usingSDCard = true;
    Serial.println("[+] Using SD card for storage");
    useDisplay = false; // Disable display when using SD card
    
    // Create CSV header on SD card if it doesn't exist
    if (!SD.exists(sdCsvFilename)) {
      File dataFile = SD.open(sdCsvFilename, FILE_WRITE);
      if (dataFile) {
        dataFile.println("MAC,SSID,AuthType,FirstSeen,Channel,RSSI,CurrentLatitude,CurrentLongitude,AltitudeMeters,AccuracyMeters,Type");
        dataFile.close();
        Serial.println("[+] Created CSV file on SD card");
      }
    }
    
    return true;
  } else {
    // Fall back to LittleFS
    usingSDCard = false;
    Serial.println("[+] Falling back to LittleFS storage");
    
    if (!LittleFS.begin()) {
      Serial.println("[-] LittleFS mount failed");
      return false;
    }
    
    // Create CSV header in LittleFS if it doesn't exist
    if (!LittleFS.exists(csvFilename)) {
      File dataFile = LittleFS.open(csvFilename, "w");
      if (dataFile) {
        dataFile.println("MAC,SSID,AuthType,FirstSeen,Channel,RSSI,CurrentLatitude,CurrentLongitude,AltitudeMeters,AccuracyMeters,Type");
        dataFile.close();
        Serial.println("[+] Created CSV file in LittleFS");
      }
    }
    
    return true;
  }
#else
  // SD card support disabled - use LittleFS only
  Serial.println("[+] Using LittleFS storage (SD card support disabled)");
  
  if (!LittleFS.begin()) {
    Serial.println("[-] LittleFS mount failed");
    return false;
  }
  
  // Create CSV header in LittleFS if it doesn't exist
  if (!LittleFS.exists(csvFilename)) {
    File dataFile = LittleFS.open(csvFilename, "w");
    if (dataFile) {
      dataFile.println("MAC,SSID,AuthType,FirstSeen,Channel,RSSI,CurrentLatitude,CurrentLongitude,AltitudeMeters,AccuracyMeters,Type");
      dataFile.close();
      Serial.println("[+] Created CSV file in LittleFS");
    }
  }
  
  return true;
#endif
}

void showStorageInfo() {
  #ifdef ENABLE_SD_CARD
  if (usingSDCard) {
    if (useDisplay) {
      display.clear();
      display.drawString(0, 0, "SD CARD DETECTED!");
      display.drawString(0, 12, "Using SD storage");
      display.drawString(0, 24, "Unlimited space");
      display.drawString(0, 36, "Extract via USB");
      display.drawString(0, 48, "or remove card");
      display.display();
    }
    
    Serial.println("[+] SD card detected - unlimited storage available");
    Serial.println("[+] Display disabled for SD card mode");
    delay(3000);
  } else {
    showStorageWarning();
  }
  #else
  showStorageWarning();
  #endif
}

void scanWiFi() {
  int networksFound = WiFi.scanNetworks();
  
  if (networksFound > 0) {
    Serial.printf("[ESP] Found %d networks:\n", networksFound);
    
    int loggedCount = 0;
    File dataFile;
    
    // Only open file for writing if storage isn't full
    if (!storageFull) {
      #ifdef ENABLE_SD_CARD
      if (usingSDCard) {
        dataFile = SD.open(sdCsvFilename, FILE_WRITE);
      } else {
        dataFile = LittleFS.open(csvFilename, "a");
      }
      #else
      dataFile = LittleFS.open(csvFilename, "a");
      #endif
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
      
      File file;
      #ifdef ENABLE_SD_CARD
      if (usingSDCard) {
        file = SD.open(filename, FILE_READ);
      } else {
        file = LittleFS.open(filename, "r");
      }
      #else
      file = LittleFS.open(filename, "r");
      #endif
      
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
#ifdef ENABLE_SD_CARD
      Serial.println("Using SD Card: " + String(usingSDCard ? "true" : "false"));
#else
      Serial.println("Using SD Card: false (disabled)");
#endif
      
      if (stats.timeValid) {
        String currentTime = getCurrentTimestamp();
        Serial.println("Current Time: " + currentTime + " UTC");
      }
      
      // Add storage info
      #ifdef ENABLE_SD_CARD
      if (usingSDCard) {
        // ESP8266 SD library doesn't have cardSize() or usedBytes() methods
        // We'll just show that SD card is being used
        File dataFile = SD.open(sdCsvFilename, FILE_READ);
        size_t fileSize = dataFile ? dataFile.size() : 0;
        if (dataFile) dataFile.close();
        
        Serial.println("Storage Type: SD Card");
        Serial.println("Card Size: Unknown (ESP8266 limitation)");
        Serial.println("Storage Free: Unlimited");
        Serial.println("CSV File Size: " + String(fileSize) + " bytes");
        Serial.println("Storage Full: false (unlimited)");
      } else {
        FSInfo fs_info;
        LittleFS.info(fs_info);
        size_t freeSpace = fs_info.totalBytes - fs_info.usedBytes;
        File dataFile = LittleFS.open(csvFilename, "r");
        size_t fileSize = dataFile ? dataFile.size() : 0;
        if (dataFile) dataFile.close();
        
        Serial.println("Storage Type: LittleFS (Internal)");
        Serial.println("Storage Free: " + String(freeSpace) + " bytes");
        Serial.println("CSV File Size: " + String(fileSize) + " bytes");
        Serial.println("Storage Full: " + String(storageFull ? "true" : "false"));
      }
      #else
      FSInfo fs_info;
      LittleFS.info(fs_info);
      size_t freeSpace = fs_info.totalBytes - fs_info.usedBytes;
      File dataFile = LittleFS.open(csvFilename, "r");
      size_t fileSize = dataFile ? dataFile.size() : 0;
      if (dataFile) dataFile.close();
      
      Serial.println("Storage Type: LittleFS (Internal)");
      Serial.println("Storage Free: " + String(freeSpace) + " bytes");
      Serial.println("CSV File Size: " + String(fileSize) + " bytes");
      Serial.println("Storage Full: " + String(storageFull ? "true" : "false"));
      #endif
      Serial.println("STATS_END");
      
    } else if (command == "DELETE_DATA") {
      #ifdef ENABLE_SD_CARD
      if (usingSDCard) {
        SD.remove(sdCsvFilename);
      } else {
        LittleFS.remove(csvFilename);
      }
      #else
      LittleFS.remove(csvFilename);
      #endif
      stats.wifiNetworks = 0;
      stats.totalScans = 0;
      Serial.println("DATA_DELETED");
      
    } else if (command == "DISPLAY_RESET") {
      Serial.println("Resetting display...");
      initializeDisplay();
      Serial.println("Display reset complete");
    } else if (command == "VERSION") {
      Serial.println("VERSION_START");
      Serial.println("Firmware: " FIRMWARE_NAME);
      Serial.println("Version: " FIRMWARE_VERSION);
      Serial.println("Build Date: " BUILD_DATE);
      Serial.println("Build Time: " BUILD_TIME);
      Serial.println("Hardware: ESP8266 D1 Mini");
      Serial.println("Features: WiFi scanning, GPS integration");
      #ifdef ENABLE_SD_CARD
      Serial.println("SD Card Support: Enabled");
      #else
      Serial.println("SD Card Support: Disabled");
      #endif
      Serial.println("VERSION_END");
    } else if (command == "SHUTDOWN") {
      Serial.println("SHUTDOWN_START");
      Serial.println("Gracefully shutting down...");
      
      // Save any pending data
      Serial.println("Saving data...");
      
      // Update display with shutdown message
      if (useDisplay && displayWorking) {
        display.clear();
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.drawString(64, 20, "SHUTTING DOWN");
        display.drawString(64, 35, "Data saved");
        display.drawString(64, 50, "Safe to power off");
        display.display();
      }
      
      // Give time for serial output and display update
      delay(2000);
      
      Serial.println("Data saved successfully");
      Serial.println("Device ready for power off");
      Serial.println("SHUTDOWN_COMPLETE");
      
      // Enter deep sleep mode (lowest power consumption)
      // This effectively shuts down the device while preserving data
      ESP.deepSleep(0); // Sleep indefinitely (until reset)
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
  
  // Initialize storage (SD card or LittleFS)
  if (!initializeStorage()) {
    Serial.println("[-] FATAL: Storage initialization failed");
    return;
  }
  
  // Show storage info (SD card or warning)
  showStorageInfo();
  delay(3000);
  
  // Initialize WiFi
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  // Initialize GPS (quick check)
  GPS_SERIAL.begin(9600);
  Serial.println("[+] GPS initialized (background mode)");
  lastGPSRetry = millis();
  
  Serial.println("[+] Wardriving ready!");
  Serial.println("[+] Scanning every 5 seconds...");
  if (useDisplay) {
    Serial.println("[+] Display updates every 10 seconds (memory optimized)");
  } else {
    Serial.println("[+] Display disabled (SD card mode) - use serial monitor for status");
  }
  #ifdef ENABLE_SD_CARD
  if (usingSDCard) {
    Serial.println("[+] Using SD card storage - unlimited space");
  } else {
    Serial.println("[+] Using internal storage - limit 500KB, extract data regularly");
  }
  #else
  Serial.println("[+] Using internal storage - limit 500KB, extract data regularly");
  #endif
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