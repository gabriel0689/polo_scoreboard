#ifndef WEB_ROUTES_H
#define WEB_ROUTES_H

#include <ESPAsyncWebServer.h>
#include "WebPages.h"

extern AsyncWebServer server;

void setupWebRoutes() {
    // Handle root URL - Scoreboard display
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", INDEX_HTML);
    });
    
    // Handle debug URL
    server.on("/debug", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", DEBUG_HTML);
    });
    
    // Handle settings URL
    server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", SETTINGS_HTML);
    });
    
    // Handle not found
    server.onNotFound([](AsyncWebServerRequest *request) {
        request->redirect("/");
    });
}

#endif 