// raw data structure sample:
// �� 0 T2 12 00 99 01 03  
//     ^ ^  ^  ^  ^  ^  ^
//     | |  |  |  |  |  └─ Away score
//     | |  |  |  |  └─── Home score
//     | |  |  |  └────── Milliseconds
//     | |  |  └───────── Seconds
//     | |  └──────────── Minutes
//     | └─────────────── Device status (T/D with a number)
//     └───────────────── Channel
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

    // Modified: Longer timeout to ensure complete messages
    static const unsigned long BYTE_TIMEOUT = 150; // ms between bytes
    bool lookingForStart = false; // Changed to false to accept any data initially
    unsigned long lastByteTime = 0;
    unsigned long lastValidDataTime = 0;

    struct ScoreData {
        char timeFormatted[6] = "00:00";
        char homeScore[3] = "00";
        char awayScore[3] = "00";
        int channel = 0;
    } scoreData, previousData;
    
    // Device status indicators
    char deviceType = 'D';      // 'D' or 'T'
    char deviceNumber = '0';    // Number after device type

    

    bool isDataValid() {
        // Check that time is properly formatted
        if (strlen(scoreData.timeFormatted) != 5) return false;
        if (scoreData.timeFormatted[2] != ':') return false;
        
        if (!isDigit(scoreData.timeFormatted[0]) || 
            !isDigit(scoreData.timeFormatted[1]) || 
            !isDigit(scoreData.timeFormatted[3]) || 
            !isDigit(scoreData.timeFormatted[4])) {
            if (debug) debugWS("Invalid time format");
            return false;
        }
        
        // Check that scores are digits
        if (strlen(scoreData.homeScore) != 2 || strlen(scoreData.awayScore) != 2) return false;
        
        if (!isDigit(scoreData.homeScore[0]) || !isDigit(scoreData.homeScore[1]) ||
            !isDigit(scoreData.awayScore[0]) || !isDigit(scoreData.awayScore[1])) {
            if (debug) debugWS("Invalid score format");
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

    bool isSpace(char c) {
        return c == ' ' || c == '\t';
    }

    // Enhanced parsing method specifically for the observed format: �1D21200990401
    void parseMessageFormat() {
        // Build debugging info for WebSocket
        String hexOutput = "Raw hex: ";
        String asciiOutput = "Raw ASCII: ";
        for (int i = 0; i < message_pos; i++) {
            char buf[5];
            snprintf(buf, sizeof(buf), "%02X ", (unsigned char)message[i]);
            hexOutput += buf;
            
            if (message[i] >= 32 && message[i] <= 126) {
                asciiOutput += message[i];
            } else {
                asciiOutput += ".";
            }
        }
        
        if (debug) {
            debugWS(hexOutput);
            debugWS(asciiOutput);
        }
        
        // More aggressive pattern search - specifically for format: �1D21200990401
        int dataStart = -1;
        
        // First, try the format where digit is followed directly by D or T
        for (int i = 0; i < message_pos - 1; i++) {
            if (isDigit(message[i]) && (message[i+1] == 'D' || message[i+1] == 'T')) {
                dataStart = i;
                if (debug) debugWS("Standard pattern found at position " + String(dataStart));
                break;
            }
        }
        
        // If that fails, try looking for control character followed by digit then D or T
        if (dataStart == -1) {
            for (int i = 0; i < message_pos - 2; i++) {
                if (message[i] < 32 && isDigit(message[i+1]) && 
                (message[i+2] == 'D' || message[i+2] == 'T')) {
                    dataStart = i + 1; // Skip control char
                    if (debug) debugWS("Control+digit+D/T pattern found at position " + String(dataStart));
                    break;
                }
            }
        }
        
        // Couldn't find the pattern
        if (dataStart == -1) {
            if (debug) debugWS("No pattern found");
            return;
        }
        
        // Extract components based on the known format
        scoreData.channel = message[dataStart] - '0';
        deviceType = message[dataStart + 1];
        deviceNumber = message[dataStart + 2]; // Get number after T/D
        
        // Need enough characters after dataStart
        if (dataStart + 13 >= message_pos) {
            if (debug) debugWS("Message too short after pattern");
            return;
        }
        
        // Minutes (positions 3-4 after dataStart)
        char minutes[3] = {message[dataStart + 3], message[dataStart + 4], '\0'};
        
        // Seconds (positions 5-6 after dataStart)
        char seconds[3] = {message[dataStart + 5], message[dataStart + 6], '\0'};
        
        // Format time
        snprintf(scoreData.timeFormatted, sizeof(scoreData.timeFormatted), "%s:%s", minutes, seconds);
        
        // Skip the milliseconds (positions 7-9)
        
        // Home score (positions 10-11)
        scoreData.homeScore[0] = message[dataStart + 9];
        scoreData.homeScore[1] = message[dataStart + 10];
        scoreData.homeScore[2] = '\0';
        
        // Away score (positions 12-13)
        if (dataStart + 12 < message_pos) {
            scoreData.awayScore[0] = message[dataStart + 11];
            scoreData.awayScore[1] = message[dataStart + 12];
            scoreData.awayScore[2] = '\0';
        }
        
        if (debug) {
            String msgInfo = "Parsed - Channel: " + String(scoreData.channel) + 
                        ", Type: " + String(deviceType) + String(deviceNumber) + 
                        ", Time: " + String(scoreData.timeFormatted) + 
                        ", Home: " + String(scoreData.homeScore) + 
                        ", Away: " + String(scoreData.awayScore);
            debugWS(msgInfo);
        }
    }

    bool hasDataChanged() {
        bool changed = strcmp(scoreData.timeFormatted, previousData.timeFormatted) != 0 ||
                      strcmp(scoreData.homeScore, previousData.homeScore) != 0 ||
                      strcmp(scoreData.awayScore, previousData.awayScore) != 0 ||
                      scoreData.channel != previousData.channel;
                      
        // More permissive - don't require device type change
        
        if (changed && debug) {
            debugWS("Data changed detected");
        }
        
        return changed;
    }

    void updatePreviousState() {
        strcpy(previousData.timeFormatted, scoreData.timeFormatted);
        strcpy(previousData.homeScore, scoreData.homeScore);
        strcpy(previousData.awayScore, scoreData.awayScore);
        previousData.channel = scoreData.channel;
    }

    void sendWebSocketUpdate() {
        // Only send data if it's valid
        if (!isDataValid()) {
            if (debug) debugWS("Data validation failed - not sending WebSocket update");
            return;
        }
        
        if (ws.count() > 0) {
            // Continue with the existing WebSocket code
            StaticJsonDocument<200> doc;
            doc["time"] = scoreData.timeFormatted;
            doc["home"] = scoreData.homeScore;
            doc["away"] = scoreData.awayScore;
            doc["deviceType"] = String(deviceType) + String(deviceNumber);
            doc["channel"] = scoreData.channel;
            doc["isRunning"] = (deviceType == 'T');
            doc["source"] = "scoreboard"; // Add source to distinguish from test data

            String jsonString;
            serializeJson(doc, jsonString);

            try {
                ws.textAll(jsonString);
                if (debug) {
                    debugWS("WS sent: " + jsonString);
                }
            } catch (...) {
                if (debug) {
                    debugWS("WebSocket send failed");
                }
            }
        }
    }

    void processMessage() {
        // Use the new format-based parsing method
        parseMessageFormat();
        
        // Only send WebSocket update if data has changed AND is valid
        if (hasDataChanged()) {
            // Validate data before sending
            if (isDataValid()) {
                sendWebSocketUpdate();
                updatePreviousState();
                lastValidDataTime = millis(); // Update this timestamp when valid data is processed
                
                if (debug) {
                    String info = "Updated - Time: " + String(scoreData.timeFormatted) + 
                                ", Home: " + String(scoreData.homeScore) + 
                                ", Away: " + String(scoreData.awayScore) + 
                                ", Type: " + String(deviceType) + String(deviceNumber);
                    debugWS(info);
                }
            } else if (debug) {
                debugWS("Invalid data detected - update skipped");
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
        Serial1.begin(9600, SERIAL_8N1, 19, 20);
        Serial1.setTimeout(50); // Add timeout for reading
        
        unsigned long startTime = millis();
        while (!Serial1 && (millis() - startTime < 1000)) {
            delay(10);
        }

        // Flush any leftover data
        while (Serial1.available()) {
            Serial1.read();
        }

        return Serial1;
    }

    void debugWS(const String& message) {
        if (!debug) return;
        
        StaticJsonDocument<256> doc;
        doc["type"] = "debug";
        doc["message"] = message;
        
        String jsonString;
        serializeJson(doc, jsonString);
        
        if (ws.count() > 0) {
            ws.textAll(jsonString);
        }
    }

    void detectBaudRate() {
        static int baudIndex = 0;
        static unsigned long lastChange = 0;
        static int baudRates[] = {9600, 19200, 38400, 57600, 115200};
        static int failedAttempts = 0;
        
        // If we've received valid data recently (within 60 seconds), don't change baud rate
        if (millis() - lastValidDataTime < 60000) {
            failedAttempts = 0; // Reset failure counter when we're getting valid data
            return;
        }
        
        // Only try changing baud rate every 15 seconds
        if (millis() - lastChange < 15000) {
            return;
        }
        
        // Check if we need to try another baud rate
        if (Serial1.available() == 0) {
            // If we've cycled through all baud rates multiple times with no success
            // and haven't received valid data in 2 minutes, go back to default
            if (failedAttempts > 10 && millis() - lastValidDataTime > 120000) {
                baudIndex = 0; // Reset to 9600 (default rate)
                failedAttempts = 0;
                if (debug) {
                    debugWS("Too many failures - resetting to default baud rate: 9600");
                }
            } else {
                baudIndex = (baudIndex + 1) % 5;
                failedAttempts++;
            }
            
            Serial1.end();
            delay(100);
            Serial1.begin(baudRates[baudIndex], SERIAL_8N1, 19, 20);
            
            if (debug) {
                debugWS("Trying baud rate: " + String(baudRates[baudIndex]));
            }
            lastChange = millis();
        } else if (debug) {
            debugWS("Data detected at current baud rate");
        }
    }

    void handleData() {
        unsigned long currentTime = millis();

        static unsigned long lastCheckTime = 0;
        if (currentTime - lastCheckTime > 5000) { // Every 5 seconds
            if (debug) {
                String status = "Serial status - Available: " + String(Serial1.available()) + " bytes";
                debugWS(status);
            }
            lastCheckTime = currentTime;
        }

        // Process data in chunks if enough is available
        if (Serial1.available() >= 13) { // We need at least a complete message
            clearBuffer();
            unsigned long readStart = millis();
            
            // Read up to a complete message
            while (Serial1.available() > 0 && message_pos < MAX_MESSAGE_LENGTH - 1 && 
                   (millis() - readStart < 50)) { // 50ms max reading time
                message[message_pos] = Serial1.read();
                message_pos++;
            }
            
            message[message_pos] = '\0';
            
            // Process this chunk immediately
            if (message_pos >= 13) {
                if (debug) debugWS("Processing complete message chunk");
                processMessage();
                clearBuffer();
            }
        }
        
        // Check for timeout on any remaining partial data
        if (message_pos > 0 && (currentTime - lastByteTime > BYTE_TIMEOUT)) {
            message[message_pos] = '\0';
            
            // Only process complete messages
            if (message_pos >= 13) {
                if (debug) debugWS("Processing message after timeout");
                processMessage();
            } else if (debug) {
                debugWS("Message too short after timeout: " + String(message_pos) + " bytes");
            }
            
            clearBuffer();
        }
        
        // Read any additional bytes (if not enough for a chunk)
        while (Serial1.available() > 0 && message_pos < MAX_MESSAGE_LENGTH - 1) {
            char inByte = Serial1.read();
            lastByteTime = currentTime;
            
            message[message_pos] = inByte;
            message_pos++;
        }
    }

    void sendTestData() {
        // Generate test data
        strcpy(scoreData.timeFormatted, "12:34");
        strcpy(scoreData.homeScore, "05");
        strcpy(scoreData.awayScore, "03");
        scoreData.channel = 1;
        deviceType = 'T';
        deviceNumber = '2';
        
        // Send to WebSocket
        if (ws.count() > 0) {
            StaticJsonDocument<200> doc;
            doc["time"] = scoreData.timeFormatted;
            doc["home"] = scoreData.homeScore;
            doc["away"] = scoreData.awayScore;
            doc["deviceType"] = String(deviceType) + String(deviceNumber);
            doc["channel"] = scoreData.channel;
            doc["isRunning"] = true;
            doc["source"] = "test"; // Identify as test data
            
            String jsonString;
            serializeJson(doc, jsonString);
            
            try {
                ws.textAll(jsonString);
                if (debug) {
                    debugWS("Test data sent via WebSocket");
                }
            } catch (...) {
                if (debug) {
                    debugWS("Test data WebSocket send failed");
                }
            }
        }
        
        // No need to update previous state here - test data is temporary
    }

    bool hasReceivedValidData(unsigned long timeout) {
        return (millis() - lastValidDataTime < timeout);
    }

    bool getDebug() const {
        return debug;
    }

    void sendCurrentState() {
        if (isDataValid()) {
            sendWebSocketUpdate();
            if (debug) {
                debugWS("Current state sent to client");
            }
        }
    }

    char getDeviceType() const { return deviceType; }
    bool isTimeRunning() const { return deviceType == 'T'; }
    
    String getTimeFormatted() const { 
        return String(scoreData.timeFormatted); 
    }
    
    String getHomeScore() const { 
        return String(scoreData.homeScore); 
    }
    
    String getAwayScore() const { 
        return String(scoreData.awayScore); 
    }
    
    int getChannel() const {
        return scoreData.channel;
    }

    void setDebug(bool enabled) {
        debug = enabled;
    }

};

extern SerialHandler serialHandler;

#endif