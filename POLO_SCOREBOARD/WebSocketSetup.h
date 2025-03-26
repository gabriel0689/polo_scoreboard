#ifndef WEBSOCKET_SETUP_H
#define WEBSOCKET_SETUP_H

#include <AsyncWebSocket.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include "WebRoutes.h"  // Add this line

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// Create WebSocket object
AsyncWebSocket ws("/ws");

// Function declarations
void setupWebSocket();
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
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
            handleWebSocketMessage(arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0;
        String message = (char*)data;
        Serial.printf("Received WebSocket message: %s\n", message.c_str());
        
        // Echo the message back to all clients
        ws.textAll(message);
    }
}


void setupWebSocket() {
    // Setup mDNS responder
    if (MDNS.begin("scoreboard")) {
        Serial.println("mDNS responder started");
    } else {
        Serial.println("Error setting up mDNS responder.");
    }

    // Attach WebSocket handler
    ws.onEvent(onEvent);
    server.addHandler(&ws);

    // Setup web routes
    setupWebRoutes();

    server.begin();
}

// Call this in your loop() function to clean up disconnected clients
void cleanupWebSocket() {
    ws.cleanupClients();
}

#endif // WEBSOCKET_SETUP_H