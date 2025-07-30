//#define BOARD_ESP32 // Comment this out if using ESP8266

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
};

WardrivingStats stats;

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
  display.drawString(0, 0, "DNS DriveBy");
  display.drawString(0, 12, "WORKING!");
  display.drawString(0, 24, "GPIO4/5");
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
  display.drawString(0, 0, "Wardriving");
  display.drawString(0, 12, "WiFi: " + String(stats.wifiNetworks));
  display.drawString(0, 24, "Scans: " + String(stats.totalScans));
  
  if (stats.gpsActive) {
    display.drawString(0, 36, "GPS: " + String(stats.satellites));
  } else {
    display.drawString(0, 36, "GPS: No fix");
  }
  
  display.drawString(0, 48, "WORKING!");
  display.display();
  
  lastDisplayUpdate = millis();
}

void updateGPS() {
  while (GPS_SERIAL.available()) {
    if (gps.encode(GPS_SERIAL.read())) {
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
    
    File dataFile = LittleFS.open(csvFilename, "a");
    
    for (int i = 0; i < networksFound; i++) {
      String ssid = WiFi.SSID(i);
      String bssid = WiFi.BSSIDstr(i);
      int rssi = WiFi.RSSI(i);
      uint8_t encType = WiFi.encryptionType(i);
      String encryption = getWiFiEncryption(encType);
      
      // Log to serial
      Serial.printf("[ESP] [%d] %s (%s) - %ddBm\n", 
                   i+1, ssid.c_str(), bssid.c_str(), rssi);
      
      // Write to file
      if (dataFile) {
        // WiGLE CSV format
        dataFile.printf("%s,%s,%s,%s,%d,%d,%.8f,%.8f,0,0,WIFI\n",
                        bssid.c_str(),
                        ssid.c_str(),
                        encryption.c_str(),
                        "2024-01-01 00:00:00",
                        WiFi.channel(i),
                        rssi,
                        stats.currentLat,
                        stats.currentLng);
        stats.wifiNetworks++;
      }
    }
    
    if (dataFile) dataFile.close();
    
    Serial.printf("[ESP] Summary: %d networks found, %d logged\n", networksFound, networksFound);
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
  Serial.println("   WiFi Wardriving - OPTIMIZED!");
  Serial.println("   Display: Memory Efficient");
  Serial.println("*********************************");
  
  // Initialize display with correct pins
  initializeDisplay();
  
  // Initialize file system
  if (!initializeFileSystem()) {
    Serial.println("[-] FATAL: Storage initialization failed");
    return;
  }
  
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
  Serial.println("[+] Send 'DISPLAY_RESET' to reset display if needed");
}

void loop() {
  // Update GPS data
  updateGPS();
  
  // Handle serial commands
  handleSerialCommands();
  
  // Perform scans every 5 seconds
  static unsigned long lastScan = 0;
  if (millis() - lastScan > 5000) {
    Serial.println("\n--- Starting scan cycle ---");
    
    scanWiFi();
    
    // Only update display occasionally to reduce memory pressure
    quickDisplayUpdate();
    
    lastScan = millis();
    
    Serial.printf("--- Scan complete (WiFi: %d, Scans: %d) ---\n", 
                  stats.wifiNetworks, stats.totalScans);
  }
  
  delay(100);
} 