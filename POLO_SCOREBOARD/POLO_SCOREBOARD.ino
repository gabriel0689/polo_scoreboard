#include <TFT_eSPI.h>
#include <WiFiManager.h>

// Local project headers
#include "DisplaySetup.h"
#include "WiFiSetup.h"
#include "WebSocketSetup.h"
#include "SerialHandler.h"  
#include "ButtonHandler.h"
#include <Preferences.h>

// Initialize components
TFT_eSPI tft = TFT_eSPI();
WiFiManager wm;
SerialHandler serialHandler;
ButtonHandler buttonHandler;
Preferences preferences;

bool systemInitialized = false;
bool displayingScoreboard = false;
bool displayingWebsiteURL = false;
unsigned long lastScoreboardUpdate = 0;
const unsigned long SCOREBOARD_UPDATE_INTERVAL = 5000; // 5 seconds

bool inConfigPortalMode = false;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize preferences
  preferences.begin("scoreboard", false); // false = read/write mode
  // Load debug setting
  bool debugMode = preferences.getBool("debugMode", false); // default to false if not set
  serialHandler.setDebug(debugMode);

  initDisplay();
  displayMessage("Initializing...");
  buttonHandler.begin();
  buttonHandler.setCallback(handleButtonPress);
}

void loop() {
  // Check if in config portal mode
  if (inConfigPortalMode) {
    // Don't do anything that would override the AP display
    delay(100);
    return;
  }

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
      delay(1000); // Give WiFi time to stabilize

    }
    
    // Now that WiFi is connected, set up WebSocket
    displayMessage("Starting WebSocket...");
    setupWebSocket();
    delay(2000); // Add delay to let the WebSocket server initialize properly
    
    
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

    // Show the website URL screen after initialization
    displayWebsiteURL();
    displayingWebsiteURL = true;
    return;
  }
  
  // Normal operation - only when fully initialized
  if (WiFi.status() != WL_CONNECTED) {
    systemInitialized = false; // Force re-initialization
    displayMessage("WiFi disconnected!");
    return;
  }

  static unsigned long lastUpdateTime = 0;
  static unsigned long lastBaudCheck = 0;
  static unsigned long lastResetCheck = 0;
  unsigned long currentMillis = millis();

  // Periodic state broadcast
  if (currentMillis - lastUpdateTime > 30000) { // Every 30 seconds
    serialHandler.sendCurrentState();
    lastUpdateTime = currentMillis;
  }

  // Baud rate checking (less frequent)
  if (currentMillis - lastBaudCheck > 60000) { // Every 60 seconds
    serialHandler.detectBaudRate();
    lastBaudCheck = currentMillis;
  }

  // Reset Serial after extended garbled data (failsafe)
  if (currentMillis - lastResetCheck > 300000) { // Every 5 minutes
    if (!serialHandler.hasReceivedValidData(180000)) { // No valid data for 3 minutes
      if (serialHandler.getDebug()) {
        serialHandler.debugWS("No valid data for extended period - resetting serial");
      }
      serialHandler.begin(); // Reinitialize the serial connection
    }
    lastResetCheck = currentMillis;
  }

  // Update scoreboard display if active
  if (displayingScoreboard) {
    if (currentMillis - lastScoreboardUpdate >= SCOREBOARD_UPDATE_INTERVAL) {
      displayScoreData();
      lastScoreboardUpdate = currentMillis;
    }
  }

  buttonHandler.update();
  serialHandler.handleData();
  cleanupWebSocket();
  delay(10);
}

void handleButtonPress(uint8_t button, ButtonPressType type) {
  if (button == 0) {  // Top button (BUTTON_1)
    if (type == SHORT_PRESS) {
      if (displayingScoreboard) {
        // Switch from scoreboard to website URL
        serialHandler.debugWS("Top button short press - showing website URL");
        displayWebsiteURL();
        displayingScoreboard = false;
        displayingWebsiteURL = true;
      } else if (displayingWebsiteURL) {
        // Switch from website URL to scoreboard
        serialHandler.debugWS("Top button short press - showing score data");
        displayScoreData();
        lastScoreboardUpdate = millis(); // Reset timer for updates
        displayingScoreboard = true;
        displayingWebsiteURL = false;
      } else {
        // Neither is showing, default to scoreboard
        serialHandler.debugWS("Top button short press - showing score data");
        displayScoreData();
        lastScoreboardUpdate = millis(); // Reset timer for updates
        displayingScoreboard = true;
      }
    } else if (type == LONG_PRESS) {
      // Handle standard long press (1 second)
      serialHandler.debugWS("Top button long press detected");
    } else if (type == VERY_LONG_PRESS) {
      // Handle very long press (10 seconds)
      serialHandler.debugWS("Top button very long press (10s) - resetting WiFi");
      
      // Show message on TFT
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.setTextDatum(MC_DATUM);
      tft.setTextSize(2);
      tft.drawString("Resetting WiFi settings", tft.width()/2, tft.height()/2);
      
      // Wait 5 seconds before reset
      delay(5000);
      
      // Reset WiFi
      WiFi.disconnect(true);
      wm.resetSettings();
      ESP.restart();
    }
  } else {  
    // Bottom button (BUTTON_2)
    if (type == SHORT_PRESS) {
      // Same toggle behavior for bottom button
      if (displayingScoreboard) {
        // Switch from scoreboard to website URL
        serialHandler.debugWS("Bottom button short press - showing website URL");
        displayWebsiteURL();
        displayingScoreboard = false;
        displayingWebsiteURL = true;
      } else if (displayingWebsiteURL) {
        // Switch from website URL to scoreboard
        serialHandler.debugWS("Bottom button short press - showing score data");
        displayScoreData();
        lastScoreboardUpdate = millis(); // Reset timer for updates
        displayingScoreboard = true;
        displayingWebsiteURL = false;
      } else {
        // Neither is showing, default to scoreboard
        serialHandler.debugWS("Bottom button short press - showing score data");
        displayScoreData();
        lastScoreboardUpdate = millis(); // Reset timer for updates
        displayingScoreboard = true;
      }
    } else if (type == LONG_PRESS) {
      // Handle long press of bottom button
      serialHandler.debugWS("Bottom button long press - resetting WiFi");
      // Reset WiFi on long press of bottom button
      WiFi.disconnect(true);
      wm.resetSettings();
      ESP.restart();
    }
  }
}


void displayScoreData() {
  // Clear screen
  tft.fillScreen(TFT_BLACK);
  
  // Set text properties for header
  tft.setTextDatum(TC_DATUM);  // Top-center alignment
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("SCOREBOARD", tft.width()/2, 5);
  
  // Display time at the top with larger font
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(3);
  tft.drawString(serialHandler.getTimeFormatted(), tft.width()/2, 25);
  
  // Draw dividing line
  tft.drawLine(0, 65, tft.width(), 65, TFT_DARKGREY);
  
  // Display team scores side by side
  int yPos = 85;
  
  // Team labels
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("HOME", tft.width()/4, yPos);
  tft.drawString("AWAY", 3*tft.width()/4, yPos);
  
  // Team scores with larger font
  yPos += 20;
  tft.setTextSize(4);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(serialHandler.getHomeScore(), tft.width()/4, yPos);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.drawString(serialHandler.getAwayScore(), 3*tft.width()/4, yPos);
  
  // Status indicator at bottom
  tft.setTextSize(1);
  tft.setTextDatum(BC_DATUM);  // Bottom-center alignment
  
  // Timer status
  tft.setTextColor(serialHandler.isTimeRunning() ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.drawString(serialHandler.isTimeRunning() ? "TIMER RUNNING" : "TIMER STOPPED", 
                 tft.width()/2, tft.height() - 15);
  
  // Source info
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString("Device: " + String(serialHandler.getDeviceType()) + 
                 "  Channel: " + String(serialHandler.getChannel()),
                 tft.width()/2, tft.height() - 5);
}

bool serverIsRunning() {
  // Simple check to see if the server is responding
  // Could use a more robust method if needed
  return WiFi.status() == WL_CONNECTED && systemInitialized;
}


void displayWebsiteURL() {
  // Clear screen
  tft.fillScreen(TFT_BLACK);
  
  // Set text properties
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM); // Middle-center alignment
  
  // Display header
  tft.setTextSize(2);
  tft.drawString("SCOREBOARD URL", tft.width()/2, 30);
  
  // Check server status
  bool isRunning = serverIsRunning();
  
  // Display URL with QR code instructions
  tft.setTextSize(1);
  tft.setTextColor(isRunning ? TFT_CYAN : TFT_RED, TFT_BLACK);
  
  // Get IP address
  String url = "http://scoreboard.local";
  
  // Draw the URL
  tft.drawString(url, tft.width()/2, tft.height()/2);
  
  // Connection status
  tft.setTextColor(isRunning ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.drawString(isRunning ? "Server Online" : "Server Starting...", 
                tft.width()/2, tft.height()/2 + 20);
  
  // Draw instructions
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("Connect to view scoreboard", tft.width()/2, tft.height()/2 + 40);
  
  // Draw network info
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString("Network: " + WiFi.SSID(), tft.width()/2, tft.height() - 30);
  tft.drawString("Press button to view scores", tft.width()/2, tft.height() - 15);
}


void displayAPInfo(const String& ssid, const String& password) {
  // Clear screen
  tft.fillScreen(TFT_BLACK);
  
  // Set text properties
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(TC_DATUM); // Top-center alignment
  
  // Display header
  tft.setTextSize(2);
  tft.drawString("WIFI SETUP", tft.width()/2, 20);
  
  // Display instructions
  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("Connect to this WiFi network:", tft.width()/2, 55);
  
  // Draw AP info
  tft.setTextDatum(MC_DATUM); // Middle-center alignment
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(ssid, tft.width()/2, tft.height()/2 - 10);
  
  // Password
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Password:", tft.width()/2, tft.height()/2 + 20);
  
  tft.setTextSize(2);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString(password, tft.width()/2, tft.height()/2 + 40);
  
  // Footer
  tft.setTextSize(1);
  tft.setTextDatum(BC_DATUM); // Bottom-center
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString("Then visit 192.168.4.1 in your browser", tft.width()/2, tft.height() - 15);
}