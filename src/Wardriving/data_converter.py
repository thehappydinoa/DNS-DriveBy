#!/usr/bin/env python3
"""
Data Converter for Wardriving Data
Converts WiGLE format to other commonly used formats
"""

import csv
import json
import argparse
import os
import sys

class WardrivingDataConverter:
    def __init__(self, input_file):
        self.input_file = input_file
        self.data = []
        self.headers = []
        
    def load_wigle_csv(self):
        """Load WiGLE format CSV data"""
        try:
            with open(self.input_file, 'r') as f:
                lines = f.readlines()
                
            # Skip WiGLE header line (first line)
            if len(lines) < 2:
                print("[-] Invalid CSV file format")
                return False
                
            # Second line contains column headers
            self.headers = lines[1].strip().split(',')
            
            # Parse data rows
            reader = csv.DictReader(lines[2:], fieldnames=self.headers)
            for row in reader:
                if any(row.values()):  # Skip empty rows
                    self.data.append(row)
                    
            print(f"[+] Loaded {len(self.data)} records from {self.input_file}")
            return True
            
        except FileNotFoundError:
            print(f"[-] File not found: {self.input_file}")
            return False
        except Exception as e:
            print(f"[-] Error loading file: {e}")
            return False
    
    def to_kismet_csv(self, output_file):
        """Convert to Kismet CSV format"""
        try:
            with open(output_file, 'w', newline='') as f:
                writer = csv.writer(f)
                
                # Kismet CSV headers
                kismet_headers = [
                    'Network', 'NetType', 'ESSID', 'BSSID', 'Info', 
                    'Channel', 'Cloaked', 'Encryption', 'Decrypted',
                    'MaxRate', 'MaxSeenRate', 'Beacon', 'LLC', 'Data',
                    'Crypt', 'Weak', 'Total', 'Carrier', 'Encoding',
                    'FirstTime', 'LastTime', 'BestQuality', 'BestSignal',
                    'BestNoise', 'GPSMinLat', 'GPSMinLon', 'GPSMinAlt',
                    'GPSMinSpd', 'GPSMaxLat', 'GPSMaxLon', 'GPSMaxAlt',
                    'GPSMaxSpd', 'GPSBestLat', 'GPSBestLon', 'GPSBestAlt',
                    'DataSize', 'IPType', 'IP'
                ]
                
                writer.writerow(kismet_headers)
                
                network_num = 1
                for row in self.data:
                    if row['Type'] == 'WIFI':
                        # Convert WiGLE row to Kismet format
                        kismet_row = [
                            network_num,  # Network
                            'infrastructure',  # NetType
                            row['SSID'],  # ESSID
                            row['MAC'].upper(),  # BSSID
                            '',  # Info
                            row['Channel'],  # Channel
                            'No',  # Cloaked
                            self._wigle_to_kismet_encryption(row['AuthMode']),  # Encryption
                            'No',  # Decrypted
                            '54.0',  # MaxRate (default)
                            '54.0',  # MaxSeenRate
                            '1',  # Beacon
                            '0',  # LLC
                            '0',  # Data
                            '0',  # Crypt
                            '0',  # Weak
                            '1',  # Total
                            '',  # Carrier
                            '',  # Encoding
                            row['FirstSeen'],  # FirstTime
                            row['FirstSeen'],  # LastTime
                            '10',  # BestQuality
                            row['RSSI'],  # BestSignal
                            '0',  # BestNoise
                            row['CurrentLatitude'],  # GPSMinLat
                            row['CurrentLongitude'],  # GPSMinLon
                            row['AltitudeMeters'],  # GPSMinAlt
                            '0',  # GPSMinSpd
                            row['CurrentLatitude'],  # GPSMaxLat
                            row['CurrentLongitude'],  # GPSMaxLon
                            row['AltitudeMeters'],  # GPSMaxAlt
                            '0',  # GPSMaxSpd
                            row['CurrentLatitude'],  # GPSBestLat
                            row['CurrentLongitude'],  # GPSBestLon
                            row['AltitudeMeters'],  # GPSBestAlt
                            '0',  # DataSize
                            '0',  # IPType
                            '0.0.0.0'  # IP
                        ]
                        writer.writerow(kismet_row)
                        network_num += 1
                        
            print(f"[+] Saved Kismet format to {output_file}")
            return True
            
        except Exception as e:
            print(f"[-] Error saving Kismet format: {e}")
            return False
    
    def to_netstumbler_txt(self, output_file):
        """Convert to NetStumbler text format"""
        try:
            with open(output_file, 'w') as f:
                f.write("# $Creator: DNS DriveBy Wardriving Data Converter\n")
                f.write("# $Format: wi-scan summary with extensions\n")
                f.write("#\n")
                f.write("# Latitude\tLongitude\t( SSID )\t\tType\tChannel\tRSSI\tVendor\tFlags\tCC\n")
                
                for row in self.data:
                    if row['Type'] == 'WIFI':
                        # NetStumbler format
                        line = f"{row['CurrentLatitude']}\t{row['CurrentLongitude']}\t" \
                               f"( {row['SSID']} )\t\t{self._wigle_to_netstumbler_type(row['AuthMode'])}\t" \
                               f"{row['Channel']}\t{row['RSSI']}\t\t{row['AuthMode']}\t\n"
                        f.write(line)
                        
            print(f"[+] Saved NetStumbler format to {output_file}")
            return True
            
        except Exception as e:
            print(f"[-] Error saving NetStumbler format: {e}")
            return False
    
    def to_geojson(self, output_file):
        """Convert to GeoJSON format for mapping"""
        try:
            features = []
            
            for row in self.data:
                lat = float(row['CurrentLatitude'])
                lng = float(row['CurrentLongitude'])
                
                feature = {
                    "type": "Feature",
                    "geometry": {
                        "type": "Point",
                        "coordinates": [lng, lat]
                    },
                    "properties": {
                        "mac": row['MAC'],
                        "ssid": row['SSID'],
                        "auth_mode": row['AuthMode'],
                        "channel": row['Channel'],
                        "rssi": int(row['RSSI']),
                        "timestamp": row['FirstSeen'],
                        "type": row['Type'].lower()
                    }
                }
                features.append(feature)
            
            geojson = {
                "type": "FeatureCollection",
                "features": features
            }
            
            with open(output_file, 'w') as f:
                json.dump(geojson, f, indent=2)
                
            print(f"[+] Saved GeoJSON format to {output_file}")
            return True
            
        except Exception as e:
            print(f"[-] Error saving GeoJSON format: {e}")
            return False
    
    def to_kml(self, output_file):
        """Convert to KML format for Google Earth"""
        try:
            kml_content = '''<?xml version="1.0" encoding="UTF-8"?>
<kml xmlns="http://www.opengis.net/kml/2.2">
  <Document>
    <name>Wardriving Data</name>
    <description>WiFi and Bluetooth devices found during wardriving</description>
    
    <!-- WiFi Style -->
    <Style id="wifiIcon">
      <IconStyle>
        <Icon>
          <href>http://maps.google.com/mapfiles/kml/pal2/icon17.png</href>
        </Icon>
      </IconStyle>
    </Style>
    
    <!-- Bluetooth Style -->
    <Style id="bluetoothIcon">
      <IconStyle>
        <Icon>
          <href>http://maps.google.com/mapfiles/kml/pal2/icon18.png</href>
        </Icon>
      </IconStyle>
    </Style>
'''
            
            for row in self.data:
                lat = row['CurrentLatitude']
                lng = row['CurrentLongitude']
                
                style = "wifiIcon" if row['Type'] == 'WIFI' else "bluetoothIcon"
                name = row['SSID'] if row['SSID'] else row['MAC']
                
                placemark = f'''
    <Placemark>
      <name>{name}</name>
      <description>
        <![CDATA[
        <b>MAC:</b> {row['MAC']}<br/>
        <b>Type:</b> {row['Type']}<br/>
        <b>Channel:</b> {row['Channel']}<br/>
        <b>RSSI:</b> {row['RSSI']} dBm<br/>
        <b>Security:</b> {row['AuthMode']}<br/>
        <b>First Seen:</b> {row['FirstSeen']}
        ]]>
      </description>
      <styleUrl>#{style}</styleUrl>
      <Point>
        <coordinates>{lng},{lat},0</coordinates>
      </Point>
    </Placemark>'''
                
                kml_content += placemark
            
            kml_content += '''
  </Document>
</kml>'''
            
            with open(output_file, 'w') as f:
                f.write(kml_content)
                
            print(f"[+] Saved KML format to {output_file}")
            return True
            
        except Exception as e:
            print(f"[-] Error saving KML format: {e}")
            return False
    
    def to_gpx(self, output_file):
        """Convert to GPX format with waypoints"""
        try:
            gpx_content = '''<?xml version="1.0" encoding="UTF-8"?>
<gpx version="1.1" creator="DNS DriveBy Wardriving Converter">
  <metadata>
    <name>Wardriving Data</name>
    <desc>WiFi and Bluetooth devices found during wardriving</desc>
  </metadata>
'''
            
            for i, row in enumerate(self.data):
                lat = row['CurrentLatitude']
                lng = row['CurrentLongitude']
                name = row['SSID'] if row['SSID'] else f"{row['Type']}_{i}"
                
                waypoint = f'''
  <wpt lat="{lat}" lon="{lng}">
    <name>{name}</name>
    <desc>MAC: {row['MAC']}, Type: {row['Type']}, RSSI: {row['RSSI']}dBm</desc>
    <type>{row['Type'].lower()}</type>
  </wpt>'''
                
                gpx_content += waypoint
            
            gpx_content += '''
</gpx>'''
            
            with open(output_file, 'w') as f:
                f.write(gpx_content)
                
            print(f"[+] Saved GPX format to {output_file}")
            return True
            
        except Exception as e:
            print(f"[-] Error saving GPX format: {e}")
            return False
    
    def _wigle_to_kismet_encryption(self, auth_mode):
        """Convert WiGLE auth mode to Kismet encryption format"""
        if '[ESS]' in auth_mode and 'WPA' not in auth_mode and 'WEP' not in auth_mode:
            return 'None'
        elif 'WEP' in auth_mode:
            return 'WEP'
        elif 'WPA2' in auth_mode:
            return 'WPA2'
        elif 'WPA' in auth_mode:
            return 'WPA'
        else:
            return 'Unknown'
    
    def _wigle_to_netstumbler_type(self, auth_mode):
        """Convert WiGLE auth mode to NetStumbler type"""
        if '[ESS]' in auth_mode:
            return 'ESS'
        else:
            return 'IBSS'
    
    def get_statistics(self):
        """Get statistics about the loaded data"""
        if not self.data:
            return None
            
        wifi_count = sum(1 for row in self.data if row['Type'] == 'WIFI')
        bt_count = sum(1 for row in self.data if row['Type'] == 'BT')
        
        # Get unique locations
        locations = set()
        for row in self.data:
            locations.add(f"{row['CurrentLatitude']},{row['CurrentLongitude']}")
        
        return {
            'total_records': len(self.data),
            'wifi_networks': wifi_count,
            'bluetooth_devices': bt_count,
            'unique_locations': len(locations)
        }

def main():
    parser = argparse.ArgumentParser(description='Convert wardriving data to different formats')
    parser.add_argument('input_file', help='Input WiGLE CSV file')
    parser.add_argument('-f', '--format', choices=['kismet', 'netstumbler', 'geojson', 'kml', 'gpx'], 
                       required=True, help='Output format')
    parser.add_argument('-o', '--output', help='Output filename (auto-generated if not specified)')
    parser.add_argument('-s', '--stats', action='store_true', help='Show statistics')
    
    args = parser.parse_args()
    
    # Generate output filename if not specified
    if not args.output:
        base_name = os.path.splitext(args.input_file)[0]
        extensions = {
            'kismet': '.csv',
            'netstumbler': '.txt', 
            'geojson': '.geojson',
            'kml': '.kml',
            'gpx': '.gpx'
        }
        args.output = f"{base_name}_{args.format}{extensions[args.format]}"
    
    # Create converter and load data
    converter = WardrivingDataConverter(args.input_file)
    
    if not converter.load_wigle_csv():
        sys.exit(1)
    
    # Show statistics if requested
    if args.stats:
        stats = converter.get_statistics()
        print("\n" + "="*40)
        print("         DATA STATISTICS")
        print("="*40)
        print(f"Total Records:      {stats['total_records']:,}")
        print(f"WiFi Networks:      {stats['wifi_networks']:,}")
        print(f"Bluetooth Devices:  {stats['bluetooth_devices']:,}")
        print(f"Unique Locations:   {stats['unique_locations']:,}")
        print("="*40)
    
    # Convert to specified format
    success = False
    if args.format == 'kismet':
        success = converter.to_kismet_csv(args.output)
    elif args.format == 'netstumbler':
        success = converter.to_netstumbler_txt(args.output)
    elif args.format == 'geojson':
        success = converter.to_geojson(args.output)
    elif args.format == 'kml':
        success = converter.to_kml(args.output)
    elif args.format == 'gpx':
        success = converter.to_gpx(args.output)
    
    if success:
        print(f"[+] Conversion completed successfully!")
        print(f"[+] Output saved to: {args.output}")
    else:
        sys.exit(1)

if __name__ == "__main__":
    main() 