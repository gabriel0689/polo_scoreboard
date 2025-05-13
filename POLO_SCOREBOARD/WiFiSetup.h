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

// Function declarations
bool setupWiFi();
bool initFS();
extern void displayMessage(String message);

// Function implementations
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

    // Try to connect using saved credentials
    if (!wm.autoConnect("LilyGo-TDisplay", "12345678")) {
        return false; 
    }

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

//todo: none of the fucking status text shows up in the display  when initializing ,could be a race condition with the initializing... message .