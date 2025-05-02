# Wi-Fi Repeater using NodeMCU ESP8266

![NodeMCU ESP8266](https://i.imgur.com/JZk9Q4l.jpg)

A configurable Wi-Fi repeater/extender using NodeMCU ESP8266 with web-based administration panel.

## Features

- Dual-mode operation (STA + AP)
- Web-based configuration interface
- WPA2 secured access point
- DHCP server for client devices
- Configuration saved in EEPROM
- System logging via web interface
- Password-protected admin panel
- Responsive web interface

## Hardware Requirements

- NodeMCU ESP8266 board
- Micro USB cable for power
- Computer for initial programming

## Installation Guide

### Flashing Process

1. **Install Required Software**:
   - Download and install [Arduino IDE](https://www.arduino.cc/en/software)
   - Add ESP8266 support:
     - Go to File > Preferences
     - Add `http://arduino.esp8266.com/stable/package_esp8266com_index.json` to Additional Boards Manager URLs
     - Install "esp8266" package from Tools > Board > Boards Manager

2. **Prepare the Firmware**:
   - Download the provided sketch (.ino file)
   - Open it in Arduino IDE
   - Select board: "NodeMCU 1.0 (ESP-12E Module)"
   - Select correct COM port

3. **Upload the Code**:
   - Connect NodeMCU via USB
   - Click the Upload button (→) in Arduino IDE
   - Wait for "Done uploading" message

### Initial Setup

1. **Connect to the Repeater**:
   - After flashing, the device will create an AP named:
     - SSID: `WiFiRepeater`
     - Password: `password123`
   - Connect your computer/phone to this network

2. **Access Web Interface**:
   - Open browser and go to: `http://192.168.4.1`
   - Login with default credentials:
     - Username: `admin`
     - Password: `admin`

3. **Configure Settings**:
   - Go to Configuration page
   - Set your main WiFi credentials (STA mode)
   - Optionally change AP settings
   - Change admin password (recommended)
   - Click Save (device will reboot)

## Usage

- The device will automatically:
  - Connect to your main WiFi network (STA)
  - Create the extended network (AP)
  - Route traffic between networks

## Troubleshooting

- If device doesn't create AP:
  - Hold FLASH button while powering on to reset
  - Re-flash the firmware
- If connection to main WiFi fails:
  - Check credentials in Configuration page
  - Ensure router is not blocking device
- For other issues:
  - Check logs in web interface
  - Serial monitor output (115200 baud)

## Advanced Configuration

You can modify these in the code before flashing:
- Default AP name/password
- Admin credentials
- IP address range
- Logging preferences

## License

MIT License - Free for personal and commercial use
