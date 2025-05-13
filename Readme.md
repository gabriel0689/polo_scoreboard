# Scoreboard Display System with LILYGO T-Display S3

## Overview
This project uses a LILYGO T-Display S3 to broadcasts the data from a scoreboard to a webServer via WebSockets.

## Hardware Requirements
- [LILYGO T-Display S3](https://www.lilygo.cc/products/t-display-s3) (ESP32-S3 with 1.9" LCD) (https://a.co/d/cWhJLKh)
- USB-A cable  that can carry data red, black, green, white
- USB-C cable for programming and power
- Waterproof Panel USB Connector, Waterproof IP68 USB Panel Mount Connector, Female to Female,

## Required Arduino Libraries
- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) - Graphics library for the display
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) - Asynchronous HTTP and WebSocket server
- [AsyncTCP](https://github.com/me-no-dev/AsyncTCP) - Asynchronous TCP library (dependency for ESPAsyncWebServer)
- [ArduinoJson](https://arduinojson.org/) - JSON processing
- [WiFiManager](https://github.com/tzapu/WiFiManager) - WiFi configuration portal

## Setup Instructions

### 1. Hardware Setup
1. Connect the LILYGO T-Display S3 to your computer via USB-C
2. The project uses pins 19 and 20 which are connected to the built-in USB Type-C port for serial communication
   - This allows data exchange with the scoreboard directly through the USB-C connection
   - No additional wiring is needed if your scoreboard can connect via USB
   - This setup disables Serial monitoring since it uses the built in usb port. When connecting to the computer the device will need to be put in boot mode to be able to be flashed. 

3. Connecting to the Scoreboard's UART Port
   - Locate the UART port on your scoreboard (in my implementation, it was on the WiFi antenna PCB)
   - Prepare a USB 2.0 Type-A cable:
     * Carefully cut the cable to expose the four internal wires
     * Identify each wire by color: Red (5V/VDD), Black (GND), Green (D-/RX), White (D+/TX)
   - Make the following connections to the scoreboard's UART pins:
     * Red wire → VDD pin (5V power)
     * Black wire → GND pin (ground)
     * Green wire → TX pin on scoreboard (connects to RX on ESP32)
     * White wire → RX pin on scoreboard (connects to TX on ESP32)
     [In the case of no connection happening please revert the tx and rx cables to debug]

   - To connect this modified cable to the ESP32:
     * Install a waterproof IP68 USB panel mount connector (female-to-female) on the scoreboard's rear panel
     * Connect the USB type A end of your modified USB cable to the internal side of this panel connector
     * Use a standard USB Type-A to Type-C cable to connect from the external panel port to the ESP32's USB-C port

### 2. TFT_eSPI Configuration
You must configure TFT_eSPI for the LILYGO T-Display S3:

1. Locate the `User_Setup_Select.h` file in your TFT_eSPI library folder
2. Comment out the default setup: `// #include <User_Setup.h>`
3. Uncomment or add: `#include <User_Setups/Setup_LILYGO_T_Display_S3.h>`

### 3. Arduino IDE Setup
1. Install ESP32 board package:
   - In Arduino IDE, go to File > Preferences
   - Add this URL to the "Additional Boards Manager URLs" field:
     `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Go to Tools > Board > Boards Manager
   - Search for "esp32" and install "ESP32 by Espressif Systems" (version 2.0.5 or newer)

2. Install all required libraries via the Arduino Library Manager

3. Select the correct board: "ESP32S3 Dev Module" or "LILYGO T-Display S3"

4. Set the following board options:
   - USB CDC On Boot: Enabled
   - USB DFU On Boot: Enabled
   - Upload Mode: UART0 / Hardware CDC
   - Flash Size: 16MB
   - Partition Scheme: 16M Flash (3MB APP/9.9MB FATFS)
   - CPU Frequency: 240MHz
   - Core Debug Level: None

### 4. Upload the Code
1. Open the `POLO_SCOREBOARD.ino` file
2. Connect the board via USB while putting the device into bootloader mode
3. Compile and upload the sketch

## Usage

### First Boot
1. On first boot, the device will create a WiFi access point named "LilyGo-TDisplay" pw "12345678"
2. Connect to this network with your phone or computer
3. A configuration portal should open automatically (or navigate to 192.168.4.1)
4. Select your home WiFi network and enter the password
5. After successful connection, the device will display its IP address on the screen

### Web Interface
Access the web interface by navigating to the device's IP address in a browser or accessing http://scoreboard.local.

The web interface has three pages:
1. **Scoreboard** (/) - Main display showing time and scores
2. **Debug** (/debug) - Shows raw WebSocket data for troubleshooting
3. **Settings** (/settings) - Configure device parameters

### Data Protocol
The device expects data from the scoreboard in the format:
<pre>
<code>
 �0 T2 12 00 99 01 03 � 
  ^ ^  ^  ^  ^  ^  ^
  | |  |  |  |  |  └─ Away score
  | |  |  |  |  └─── Home score
  | |  |  |  └────── Milliseconds
  | |  |  └───────── Seconds
  | |  └──────────── Minutes
  | └─────────────── Device status (T/D with a number)
  └───────────────── Channel
</code>
</pre>
## Troubleshooting
- If the display shows "WiFi Failed," try resetting the device and reconnecting
- If scores appear incorrect, enable debug mode using `serialHandler.setDebug(true)`
- Check the /debug page for raw data analysis
- If using both USB communication and debugging simultaneously, note that pins 19/20 are shared with the USB interface

## Technical Details for LILYGO T-Display S3
- Processor: ESP32-S3 dual-core processor
- Display: 1.9" 170×320 LCD with 8-bit color depth
- USB-C connector for power and programming
- **USB Serial Communication**: Uses pins 19 (RX) and 20 (TX) which are connected to the built-in USB-C port

## License
This project is licensed under a Creative Commons Attribution-NonCommercial 4.0 International License. CC BY-NC

You are free to:
- Share — copy and redistribute the material in any medium or format
- Adapt — remix, transform, and build upon the material

Under the following terms:
- Attribution — You must give appropriate credit, provide a link to the original project, and indicate if changes were made.
- NonCommercial — You may not use the material for commercial purposes.

This is a human-readable summary of (and not a substitute for) the full license, which can be found at: https://creativecommons.org/licenses/by-nc/4.0/legalcode