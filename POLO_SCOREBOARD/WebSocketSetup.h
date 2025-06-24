#ifndef WEBSOCKET_SETUP_H
#define WEBSOCKET_SETUP_H

#include <AsyncWebSocket.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include "WebRoutes.h"  // Add this line
#include "SerialHandler.h"
#include <ArduinoJson.h>
#include <Preferences.h>

extern Preferences preferences;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// Create WebSocket object
AsyncWebSocket ws("/ws");

// Function declarations
void setupWebSocket();
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, uint32_t clientId);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

// Function implementations
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len, client->id()); // Pass client ID as additional parameter
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, uint32_t clientId) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0;
        String message = (char*)data;
        Serial.printf("Received WebSocket message: %s\n", message.c_str());
        
        
        // Check for getCurrentData command
        if (message.indexOf("\"command\":\"getCurrentData\"") > 0) {
            Serial.println("Received getCurrentData command");
            serialHandler.sendCurrentState();
            return;
        }
        
        // Check for getSettings command
        if (message.indexOf("\"command\":\"getSettings\"") > 0) {
            // Create JSON with current settings
            DynamicJsonDocument doc(256);
            JsonObject settings = doc.createNestedObject("settings");
            settings["debugMode"] = serialHandler.getDebug();
            settings["baudRate"] = Serial.baudRate();
            
            String jsonString;
            serializeJson(doc, jsonString);
            ws.text(clientId, jsonString); // Use the clientId variable
            return;
        }
        
        // Check for debugMode setting
        if (message.indexOf("\"debugMode\":") > 0) {
            bool debugEnabled = message.indexOf("\"debugMode\":true") > 0;
            serialHandler.setDebug(debugEnabled);
            
            // Save to preferences
            preferences.putBool("debugMode", debugEnabled);
            
            String response = "{\"status\":\"success\",\"message\":\"Debug mode ";
            response += debugEnabled ? "enabled" : "disabled";
            response += "\"}";
            
            ws.text(clientId, response); // Use the clientId variable
            return;
        }
        
        // Echo the message back to all clients
        ws.textAll(message);
    }
}

void setupWebSocket() {
    // Setup mDNS responder
    if (MDNS.begin("scoreboard")) {
        Serial.println("mDNS responder started");
    } else {
        Serial.println("Error starting mDNS");
        // Try again
        delay(1000);
        MDNS.begin("scoreboard");
    }

    // Attach WebSocket handler
    ws.onEvent(onEvent);
    server.addHandler(&ws);
    MDNS.addService("http", "tcp", 80);
    // Setup web routes
    setupWebRoutes();

    server.begin();
}

// Call this in your loop() function to clean up disconnected clients
void cleanupWebSocket() {
    ws.cleanupClients();
}

#endif // WEBSOCKET_SETUP_H