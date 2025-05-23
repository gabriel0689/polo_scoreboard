#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include "DisplaySetup.h"
#include "FS.h"
#include <FFat.h> //like yo'mama

// External declarations 
extern WiFiManager wm;
extern void setupWebSocket(); 
extern TFT_eSPI tft;  // Add this to access the TFT directly in the callback

// Function declarations
bool setupWiFi();
bool initFS();
extern void displayMessage(String message);
extern bool inConfigPortalMode;

// Function to generate a unique AP name based on ESP32 chip ID
String getUniqueAPName() {
    uint32_t chipId = (uint32_t)(ESP.getEfuseMac() >> 32); // Get upper 32 bits of MAC
    return "FC-Scoreboard-" + String(chipId & 0xFFFF, HEX); // Use last 16 bits (4 hex chars)
}

void configModeCallback(WiFiManager *myWiFiManager) {
    Serial.println("Entered config mode");
    inConfigPortalMode = true;
    
    // Get the dynamic AP name
    String apName = getUniqueAPName();

    tft.fillScreen(TFT_BLACK);
    
    // Header
    tft.setTextDatum(TC_DATUM);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("WIFI SETUP", tft.width()/2, 20);
    
    // Instructions
    tft.setTextSize(1);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("Connect to this WiFi network:", tft.width()/2, 55);
    
    // SSID - use dynamic name
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString(apName, tft.width()/2, tft.height()/2 - 20);
    
    // Password
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Password:", tft.width()/2, tft.height()/2 + 15);
    
    tft.setTextSize(2);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("12345678", tft.width()/2, tft.height()/2 + 40);
    
    // Configuration instructions
    tft.setTextSize(1);
    tft.setTextDatum(BC_DATUM);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("Then visit 192.168.4.1 in your browser", tft.width()/2, tft.height() - 10);
}

// Function implementations - keeping all existing functionality
bool initFS() {
    if (!FFat.begin(true)) {
        Serial.println("FFat Mount Failed");
        return false;
    }
    Serial.println("FFat Mounted Successfully");
    return true;
}

bool setupWiFi() {
    // Initialize filesystem first
    if (!initFS()) {
        Serial.println("Warning: Filesystem initialization failed");
    }
    
    // Set the callback BEFORE autoConnect
    wm.setAPCallback(configModeCallback);

    // Get the dynamic AP name
    String apName = getUniqueAPName();
    
    // Try to connect using saved credentials
    if (!wm.autoConnect(apName.c_str(), "12345678")) {
        inConfigPortalMode = false; // Reset flag when failed
        return false; 
    }

    // Reset flag when connected
    inConfigPortalMode = false;
    
    // Rest of your existing code
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi Connected!");
        displayMessage("WiFi Connected!");
        return true;
    }
    
    // Connection failed
    displayMessage("Wi-Fi connection failed.\nRebooting...");
    delay(3000);
    ESP.restart();
    return false;
}

#endif // WIFI_SETUP_H