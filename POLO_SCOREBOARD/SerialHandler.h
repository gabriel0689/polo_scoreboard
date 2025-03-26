#ifndef SERIAL_HANDLER_H
#define SERIAL_HANDLER_H

#include <Arduino.h>
#include "WebSocketSetup.h"
#include <ArduinoJson.h>
#include "DisplaySetup.h"

extern AsyncWebSocket ws;

class SerialHandler {
private:

    static const int MAX_MESSAGE_LENGTH = 18;
    char message[MAX_MESSAGE_LENGTH];
    unsigned int message_pos = 0;
    bool debug = false;

    static const char START_MARKER = '8';  // Start marker based on your data
    static const unsigned long BYTE_TIMEOUT = 50; // ms between bytes
    bool lookingForStart = true;
    unsigned long lastByteTime = 0;

    struct ScoreData {
        char timeFormatted[6] = "00:00";
        char homeScore[3] = "00";
        char awayScore[3] = "00";
    } scoreData, previousData;

    bool isDataValid() {
        // Check that time is properly formatted
        if (strlen(scoreData.timeFormatted) != 5) return false;
        if (scoreData.timeFormatted[2] != ':') return false;
        
        if (!isDigit(scoreData.timeFormatted[0]) || 
            !isDigit(scoreData.timeFormatted[1]) || 
            !isDigit(scoreData.timeFormatted[3]) || 
            !isDigit(scoreData.timeFormatted[4])) {
            if (debug) Serial.println("Invalid time format");
            return false;
        }
        
        // Check that scores are digits
        if (strlen(scoreData.homeScore) != 2 || strlen(scoreData.awayScore) != 2) return false;
        
        if (!isDigit(scoreData.homeScore[0]) || !isDigit(scoreData.homeScore[1]) ||
            !isDigit(scoreData.awayScore[0]) || !isDigit(scoreData.awayScore[1])) {
            if (debug) Serial.println("Invalid score format");
            return false;
        }
        
        return true;
    }

    void clearBuffer() {
        message_pos = 0;
        memset(message, 0, MAX_MESSAGE_LENGTH);
    }

    bool isDigit(char c) {
        return c >= '0' && c <= '9';
    }

    void processTimeString(const char* rawMessage) {
        // Extract minutes (positions 3-4)
        char minutes[3] = {rawMessage[3], rawMessage[4], '\0'};
        
        // Extract seconds (positions 5-6)
        char seconds[3] = {rawMessage[5], rawMessage[6], '\0'};
        
        // Validate digits
        for (int i = 0; i < 2; i++) {
            if (!isDigit(minutes[i])) minutes[i] = '0';
            if (!isDigit(seconds[i])) seconds[i] = '0';
        }
        
        // Format as MM:SS
        snprintf(scoreData.timeFormatted, sizeof(scoreData.timeFormatted), 
                "%s:%s", minutes, seconds);
    }
    
    void processScores(const char* rawMessage) {
        // Extract home score (position 10)
        scoreData.homeScore[0] = '0';
        scoreData.homeScore[1] = rawMessage[10];
        scoreData.homeScore[2] = '\0';
    
        // Extract away score (position 12)
        scoreData.awayScore[0] = '0';
        scoreData.awayScore[1] = rawMessage[12];
        scoreData.awayScore[2] = '\0';
        
        // Validate digits
        if (!isDigit(scoreData.homeScore[1])) scoreData.homeScore[1] = '0';
        if (!isDigit(scoreData.awayScore[1])) scoreData.awayScore[1] = '0';
    }

    bool hasDataChanged() {
        bool changed = strcmp(scoreData.timeFormatted, previousData.timeFormatted) != 0 ||
                      strcmp(scoreData.homeScore, previousData.homeScore) != 0 ||
                      strcmp(scoreData.awayScore, previousData.awayScore) != 0;
        
        if (changed && debug) {
            Serial.println("Data changed detected");
        }
        
        return changed;
    }

    void updatePreviousState() {
        strcpy(previousData.timeFormatted, scoreData.timeFormatted);
        strcpy(previousData.homeScore, scoreData.homeScore);
        strcpy(previousData.awayScore, scoreData.awayScore);
    }

    void sendWebSocketUpdate() {
        // Only send data if it's valid
        if (!isDataValid()) {
            if (debug) Serial.println("Data validation failed - not sending WebSocket update");
            return;
        }
        
        if (ws.count() > 0) {
            // Continue with the existing WebSocket code
            StaticJsonDocument<200> doc;
            doc["time"] = scoreData.timeFormatted;
            doc["home"] = scoreData.homeScore;
            doc["away"] = scoreData.awayScore;

            String jsonString;
            serializeJson(doc, jsonString);

            try {
                ws.textAll(jsonString);
                if (debug) {
                    Serial.println("WS sent: " + jsonString);
                }
            } catch (...) {
                if (debug) {
                    Serial.println("WebSocket send failed");
                }
            }
        }
    }

    void processMessage() {
        if (debug) {
            Serial.print("Raw data: ");
            Serial.println(message);
            
            String rawDataMsg = "Raw: ";
            rawDataMsg += message;
            displayMessage(rawDataMsg);
        }
    
        // Process time from the specified positions
        processTimeString(message);
        
        // Process scores from the specified positions
        processScores(message);
    
        // Only send WebSocket update if data has changed AND is valid
        if (hasDataChanged()) {
            // Validate data before sending
            if (isDataValid()) {
                sendWebSocketUpdate();
                updatePreviousState();
                
                if (debug) {
                    Serial.printf("Updated - Time: %s, Home: %s, Away: %s\n", 
                                scoreData.timeFormatted, 
                                scoreData.homeScore, 
                                scoreData.awayScore);
                }
            } else if (debug) {
                Serial.println("Invalid data detected - update skipped");
            }
        }
    }

public:
    SerialHandler() {
        clearBuffer();
    }

    bool begin() {
        Serial1.end();
        delay(100);
        //todo: set baud rate to the one saved in preferences in settings, default 9600
        Serial1.begin(9600, SERIAL_8N1, 19, 20);
        
        unsigned long startTime = millis();
        while (!Serial1 && (millis() - startTime < 1000)) {
            delay(10);
        }

        return Serial1;
    }

    void handleData() {
        unsigned long currentTime = millis();
        
        // Check for timeout - end of message
        if (message_pos > 0 && (currentTime - lastByteTime > BYTE_TIMEOUT)) {
            message[message_pos] = '\0';
            
            // Only process complete messages (with minimum expected length)
            if (message_pos >= 13) {
                processMessage();
            } else if (debug) {
                Serial.printf("Message too short: %d bytes\n", message_pos);
            }
            
            clearBuffer();
            lookingForStart = true;
        }
        
        // Read available data
        while (Serial1.available() > 0) {
            char inByte = Serial1.read();
            lastByteTime = currentTime;
            
            // Looking for start marker
            if (lookingForStart) {
                if (inByte == START_MARKER) {
                    lookingForStart = false;
                    message[0] = inByte;
                    message_pos = 1;
                }
            }
            // Building message
            else if (message_pos < MAX_MESSAGE_LENGTH - 1) {
                message[message_pos] = inByte;
                message_pos++;
                
                // If buffer is full, process the message
                if (message_pos >= MAX_MESSAGE_LENGTH - 1) {
                    message[message_pos] = '\0';
                    processMessage();
                    clearBuffer();
                    lookingForStart = true;
                }
            }
            else {
                // Buffer overflow protection
                clearBuffer();
                lookingForStart = true;
            }
        }
    }

    // Getter methods dont remember if they are being used or not
    // const char* getHomeScore() const { return scoreData.homeScore; }
    // const char* getAwayScore() const { return scoreData.awayScore; }
    // const char* getTimeFormatted() const { return scoreData.timeFormatted; }

    void setDebug(bool enabled) {
        debug = enabled;
    }
};

extern SerialHandler serialHandler;

#endif