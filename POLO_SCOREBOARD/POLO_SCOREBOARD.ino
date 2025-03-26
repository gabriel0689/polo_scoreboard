#include <TFT_eSPI.h>
#include <WiFiManager.h>

// Local project headers
#include "DisplaySetup.h"
#include "WiFiSetup.h"
#include "WebSocketSetup.h"
#include "SerialHandler.h"  

// Initialize components
TFT_eSPI tft = TFT_eSPI();
WiFiManager wm;
SerialHandler serialHandler;
bool systemInitialized = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  initDisplay();
  displayMessage("Initializing...");
  
  // Only set up WiFi initially - everything else waits
}

void loop() {
  // System initialization phase
  if (!systemInitialized) {
    // First, ensure WiFi is connected
    if (WiFi.status() != WL_CONNECTED) {
      if (!setupWiFi()) {
        displayMessage("WiFi Failed! Retrying...");
        delay(5000);
        return; // Try again next loop iteration
      }
      displayMessage("WiFi Connected!");
    }
    
    // Now that WiFi is connected, set up WebSocket
    setupWebSocket();
    
    // Finally, initialize serial
    if (!serialHandler.begin()) {
      displayMessage("Serial Failed!");
      delay(3000);
      ESP.restart();
    }
    
    // All systems go!
    systemInitialized = true;
    displayMessage("System Ready!");
    delay(1000);
    return;
  }
  
  // Normal operation - only when fully initialized
  if (WiFi.status() != WL_CONNECTED) {
    systemInitialized = false; // Force re-initialization
    displayMessage("WiFi disconnected!");
    return;
  }
  
  // Only execute when everything is working
  serialHandler.setDebug(true);
  serialHandler.handleData();
  cleanupWebSocket();
  delay(10);
}