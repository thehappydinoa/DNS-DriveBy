#!/usr/bin/env python3
"""
Data Extractor for DNS DriveBy Wardriving Device
Extracts CSV data from ESP device and processes it for WiGLE or other tools
"""

import serial
import time
import sys
import argparse
from datetime import datetime
import math

class WardrivingDataExtractor:
    def __init__(self, port='/dev/ttyUSB0', baudrate=115200):
        self.port = port
        self.baudrate = baudrate
        self.ser = None
        
    def connect(self):
        """Connect to the ESP device"""
        try:
            self.ser = serial.Serial(self.port, self.baudrate, timeout=1)
            print(f"[+] Connected to {self.port} at {self.baudrate} baud")
            time.sleep(2)  # Wait for device to initialize
            return True
        except serial.SerialException as e:
            print(f"[-] Failed to connect to {self.port}: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from the ESP device"""
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("[+] Disconnected from device")
    
    def read_spiffs_file(self, filename='/wardrive_data.csv'):
        """
        Send commands to ESP device to read file content with improved timing
        """
        if not self.ser:
            print("[-] Not connected to device")
            return None
            
        # Send command with proper flushing
        command = f"READ_FILE:{filename}\n"
        self.ser.write(command.encode())
        self.ser.flush()
        time.sleep(2)  # Give device time to process
        
        data_lines = []
        start_time = time.time()
        timeout = 20  # 20 second timeout
        
        while time.time() - start_time < timeout:
            # Read all available data
            while self.ser.in_waiting:
                try:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        data_lines.append(line)
                        if line == 'FILE_END':
                            # Extract only CSV data (between FILE_START and FILE_END)
                            csv_lines = []
                            collecting = False
                            for data_line in data_lines:
                                if data_line == 'FILE_START':
                                    collecting = True
                                elif data_line == 'FILE_END':
                                    break
                                elif collecting:
                                    csv_lines.append(data_line)
                            return csv_lines
                except UnicodeDecodeError:
                    continue
            time.sleep(0.1)
                
        print("[-] Timeout waiting for file data")
        return None
    
    def calculate_distance(self, lat1, lon1, lat2, lon2):
        """Calculate distance between two GPS coordinates in meters"""
        if lat1 == 0 or lon1 == 0 or lat2 == 0 or lon2 == 0:
            return 0
            
        # Haversine formula
        R = 6371000  # Earth's radius in meters
        lat1_rad = math.radians(lat1)
        lat2_rad = math.radians(lat2)
        delta_lat = math.radians(lat2 - lat1)
        delta_lon = math.radians(lon2 - lon1)
        
        a = (math.sin(delta_lat / 2) * math.sin(delta_lat / 2) +
             math.cos(lat1_rad) * math.cos(lat2_rad) *
             math.sin(delta_lon / 2) * math.sin(delta_lon / 2))
        c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))
        
        return R * c
    
    def deduplicate_data(self, data, distance_threshold=50, rssi_threshold=10, time_threshold=300):
        """
        Deduplicate wardriving data based on MAC address with smart criteria
        
        Args:
            data: List of CSV lines
            distance_threshold: Minimum distance change in meters to keep new entry
            rssi_threshold: Minimum RSSI change in dBm to keep new entry  
            time_threshold: Minimum time difference in seconds to keep new entry
        """
        if not data or len(data) <= 2:
            return data
            
        # Keep headers
        deduped_data = data[:2]
        seen_networks = {}  # MAC -> {last_entry, lat, lon, rssi, timestamp}
        
        for line in data[2:]:  # Skip headers
            parts = line.split(',')
            if len(parts) < 11:
                continue
                
            mac = parts[0]
            ssid = parts[1]
            rssi = int(parts[5]) if parts[5].lstrip('-').isdigit() else 0
            lat = float(parts[6]) if parts[6] and parts[6] != '0.00000000' else 0
            lon = float(parts[7]) if parts[7] and parts[7] != '0.00000000' else 0
            timestamp_str = parts[3]
            
            # Convert timestamp to seconds (simplified)
            try:
                timestamp = datetime.strptime(timestamp_str, "%Y-%m-%d %H:%M:%S").timestamp()
            except:
                timestamp = time.time()
            
            should_keep = True
            
            if mac in seen_networks:
                prev = seen_networks[mac]
                
                # Calculate differences
                distance = self.calculate_distance(prev['lat'], prev['lon'], lat, lon)
                rssi_diff = abs(rssi - prev['rssi'])
                time_diff = timestamp - prev['timestamp']
                
                # Only keep if significant change occurred
                if (distance < distance_threshold and 
                    rssi_diff < rssi_threshold and 
                    time_diff < time_threshold):
                    should_keep = False
            
            if should_keep:
                deduped_data.append(line)
                seen_networks[mac] = {
                    'lat': lat,
                    'lon': lon, 
                    'rssi': rssi,
                    'timestamp': timestamp,
                    'line': line
                }
        
        original_count = len(data) - 2
        deduped_count = len(deduped_data) - 2
        removed_count = original_count - deduped_count
        
        print(f"[+] Deduplication: {original_count} -> {deduped_count} entries ({removed_count} removed)")
        
        return deduped_data
    
    def save_data(self, data, output_file=None, dedupe=True):
        """Save extracted data to file with optional deduplication"""
        if not data:
            print("[-] No data to save")
            return False
            
        # Apply deduplication if requested
        if dedupe:
            data = self.deduplicate_data(data)
            
        if not output_file:
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            output_file = f"wardrive_data_{timestamp}.csv"
        
        try:
            with open(output_file, 'w') as f:
                for line in data:
                    f.write(line + '\n')
            print(f"[+] Data saved to {output_file}")
            return True
        except IOError as e:
            print(f"[-] Failed to save data: {e}")
            return False
    
    def validate_wigle_format(self, data):
        """Validate that the data is in proper WiGLE format"""
        if not data or len(data) < 2:
            print("[-] Insufficient data for validation")
            return False
            
        # Check WiGLE header
        header_line = data[0]
        if not header_line.startswith("WigleWifi-"):
            print("[-] Invalid WiGLE header format")
            return False
            
        # Check column headers
        column_line = data[1]
        expected_columns = ["MAC", "SSID", "AuthMode", "FirstSeen", "Channel", 
                          "RSSI", "CurrentLatitude", "CurrentLongitude", 
                          "AltitudeMeters", "AccuracyMeters", "Type"]
        
        columns = column_line.split(',')
        if columns != expected_columns:
            print("[-] Invalid column headers")
            return False
            
        print("[+] Data format validation passed")
        return True
    
    def get_statistics(self, data):
        """Generate statistics from the wardriving data"""
        if not data or len(data) <= 2:
            return None
            
        wifi_count = 0
        bluetooth_count = 0
        unique_locations = set()
        unique_networks = set()  # Track unique MAC addresses
        signal_strengths = []
        
        for line in data[2:]:  # Skip headers
            parts = line.split(',')
            if len(parts) >= 11:
                mac = parts[0]
                rssi = parts[5]
                device_type = parts[10]  # Type column
                lat_lng = f"{parts[6]},{parts[7]}"  # Lat,Lng
                
                unique_locations.add(lat_lng)
                unique_networks.add(mac)
                
                if rssi.lstrip('-').isdigit():
                    signal_strengths.append(int(rssi))
                
                if device_type == "WIFI":
                    wifi_count += 1
                elif device_type == "BT":
                    bluetooth_count += 1
        
        # Calculate signal strength statistics
        avg_rssi = sum(signal_strengths) / len(signal_strengths) if signal_strengths else 0
        min_rssi = min(signal_strengths) if signal_strengths else 0
        max_rssi = max(signal_strengths) if signal_strengths else 0
        
        stats = {
            'total_entries': len(data) - 2,
            'wifi_networks': wifi_count,
            'bluetooth_devices': bluetooth_count,
            'unique_locations': len(unique_locations),
            'unique_networks': len(unique_networks),
            'avg_rssi': avg_rssi,
            'min_rssi': min_rssi,
            'max_rssi': max_rssi
        }
        
        return stats
    
    def print_statistics(self, stats):
        """Print statistics in a formatted way"""
        if not stats:
            print("[-] No statistics available")
            return
            
        print("\n" + "="*40)
        print("         WARDRIVING STATISTICS")
        print("="*40)
        print(f"Total Entries:      {stats['total_entries']:,}")
        print(f"WiFi Networks:      {stats['wifi_networks']:,}")
        print(f"Bluetooth Devices:  {stats['bluetooth_devices']:,}")
        print(f"Unique Networks:    {stats['unique_networks']:,}")
        print(f"Unique Locations:   {stats['unique_locations']:,}")
        print(f"Average RSSI:       {stats['avg_rssi']:.1f} dBm")
        print(f"Signal Range:       {stats['min_rssi']} to {stats['max_rssi']} dBm")
        print("="*40)

def find_esp_ports():
    """Find potential ESP device ports"""
    import glob
    
    # Common ESP device patterns
    patterns = [
        '/dev/ttyUSB*',
        '/dev/ttyACM*', 
        '/dev/cu.usbserial*',
        '/dev/cu.SLAB_USBtoUART*',
        'COM*'  # Windows
    ]
    
    ports = []
    for pattern in patterns:
        ports.extend(glob.glob(pattern))
    
    return ports

def main():
    parser = argparse.ArgumentParser(description='Extract wardriving data from DNS DriveBy device')
    parser.add_argument('-p', '--port', help='Serial port (auto-detect if not specified)')
    parser.add_argument('-b', '--baudrate', type=int, default=115200, help='Baud rate (default: 115200)')
    parser.add_argument('-o', '--output', help='Output filename (auto-generated if not specified)')
    parser.add_argument('-s', '--stats', action='store_true', help='Show statistics only')
    parser.add_argument('--no-dedupe', action='store_true', help='Disable deduplication')
    parser.add_argument('--distance-threshold', type=float, default=50.0, 
                       help='Distance threshold for deduplication in meters (default: 50)')
    parser.add_argument('--rssi-threshold', type=int, default=10,
                       help='RSSI threshold for deduplication in dBm (default: 10)')
    parser.add_argument('--time-threshold', type=int, default=300,
                       help='Time threshold for deduplication in seconds (default: 300)')
    
    args = parser.parse_args()
    
    # Auto-detect port if not specified
    if not args.port:
        ports = find_esp_ports()
        if not ports:
            print("[-] No ESP devices found. Please specify port manually.")
            sys.exit(1)
        args.port = ports[0]
        print(f"[+] Auto-detected port: {args.port}")
    
    # Create extractor
    extractor = WardrivingDataExtractor(args.port, args.baudrate)
    
    try:
        # Connect to device
        if not extractor.connect():
            sys.exit(1)
        
        print("[+] Reading wardriving data...")
        data = extractor.read_spiffs_file()
        
        if not data:
            print("[-] No data received from device")
            sys.exit(1)
        
        # Validate format
        if not extractor.validate_wigle_format(data):
            print("[-] Data format validation failed")
            sys.exit(1)
        
        # Apply deduplication if not disabled
        if not args.no_dedupe:
            data = extractor.deduplicate_data(
                data, 
                args.distance_threshold, 
                args.rssi_threshold, 
                args.time_threshold
            )
        
        # Get statistics
        stats = extractor.get_statistics(data)
        extractor.print_statistics(stats)
        
        if not args.stats:
            # Save data (deduplication already applied above)
            if extractor.save_data(data, args.output, dedupe=False):
                print("[+] Data extraction completed successfully!")
            else:
                sys.exit(1)
        
    except KeyboardInterrupt:
        print("\n[!] Interrupted by user")
    finally:
        extractor.disconnect()

if __name__ == "__main__":
    main() 