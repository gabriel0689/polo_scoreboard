#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>

// T-Display S3 button pins
#define BUTTON_1 0    // Boot button (GPIO0)
#define BUTTON_2 14   // User button (GPIO14)

// Button press types
enum ButtonPressType {
    SHORT_PRESS,
    LONG_PRESS,
    VERY_LONG_PRESS
};

// Callback function type
typedef void (*ButtonCallback)(uint8_t button, ButtonPressType type);

class ButtonHandler {
private:
    // Button state variables
    bool lastButtonState[2] = {HIGH, HIGH}; // Buttons are typically pulled-up
    bool buttonState[2] = {HIGH, HIGH};
    unsigned long lastDebounceTime[2] = {0, 0};
    unsigned long buttonPressStartTime[2] = {0, 0};
    bool buttonPressed[2] = {false, false};
    
    // Very long press tracking
    bool veryLongPressActive[2] = {false, false};
    bool veryLongPressTriggered[2] = {false, false};
    
    // Configuration
    static const unsigned long DEBOUNCE_DELAY = 50;        // Debounce time in ms
    static const unsigned long LONG_PRESS_TIME = 1000;     // Long press threshold in ms
    static const unsigned long VERY_LONG_PRESS_TIME = 10000; // Very long press (10 seconds)
    
    // Callback function pointer
    ButtonCallback callback = nullptr;
    
public:
    ButtonHandler() {}
    
    void begin() {
        // Configure button pins as inputs with pull-ups
        pinMode(BUTTON_1, INPUT_PULLUP);
        pinMode(BUTTON_2, INPUT_PULLUP);
    }
    
    void setCallback(ButtonCallback cb) {
        callback = cb;
    }
    
    void update() {
        // Check both buttons
        checkButton(0, BUTTON_1);
        checkButton(1, BUTTON_2);
        
        // Check for very long press on both buttons
        checkVeryLongPress(0, BUTTON_1);
        checkVeryLongPress(1, BUTTON_2);
    }
    
    // Returns true if a button is currently being pressed
    bool isButtonPressed(uint8_t buttonIndex) {
        if (buttonIndex >= 2) return false;
        return buttonPressed[buttonIndex];
    }
    
    // Returns the duration of the current press in milliseconds
    unsigned long getPressDuration(uint8_t buttonIndex) {
        if (buttonIndex >= 2 || !buttonPressed[buttonIndex]) return 0;
        return millis() - buttonPressStartTime[buttonIndex];
    }
    
private:
    void checkButton(uint8_t index, uint8_t pin) {
        // Read the button state (inverted because of pull-up)
        bool reading = !digitalRead(pin);
        
        // Check if the button state has changed
        if (reading != lastButtonState[index]) {
            // Reset the debounce timer
            lastDebounceTime[index] = millis();
        }
        
        // Wait for debounce period
        if ((millis() - lastDebounceTime[index]) > DEBOUNCE_DELAY) {
            // If the button state has changed and is stable
            if (reading != buttonState[index]) {
                buttonState[index] = reading;
                
                // Button is pressed
                if (buttonState[index] == true) {
                    buttonPressStartTime[index] = millis();
                    buttonPressed[index] = true;
                    
                    // Start very long press tracking
                    veryLongPressActive[index] = true;
                    veryLongPressTriggered[index] = false;
                }
                // Button is released
                else if (buttonPressed[index]) {
                    buttonPressed[index] = false;
                    unsigned long pressDuration = millis() - buttonPressStartTime[index];
                    
                    // Reset very long press tracking
                    veryLongPressActive[index] = false;
                    
                    // If very long press wasn't triggered yet
                    if (!veryLongPressTriggered[index]) {
                        // Call the callback if it exists
                        if (callback != nullptr) {
                            ButtonPressType type = (pressDuration >= LONG_PRESS_TIME) ? 
                                LONG_PRESS : SHORT_PRESS;
                            callback(index, type);
                        }
                    }
                }
            }
        }
        
        // Save the current reading for next comparison
        lastButtonState[index] = reading;
    }
    
    void checkVeryLongPress(uint8_t index, uint8_t pin) {
        if (veryLongPressActive[index]) {
            // If button is still pressed
            if (!digitalRead(pin)) {
                unsigned long pressDuration = millis() - buttonPressStartTime[index];
                
                // If we've reached very long press time and haven't triggered yet
                if (pressDuration >= VERY_LONG_PRESS_TIME && !veryLongPressTriggered[index]) {
                    veryLongPressTriggered[index] = true;
                    
                    // Call the callback with VERY_LONG_PRESS type
                    if (callback != nullptr) {
                        callback(index, VERY_LONG_PRESS);
                    }
                }
            } else {
                // Button was released before reaching very long press threshold
                veryLongPressActive[index] = false;
            }
        }
    }
};

#endif // BUTTON_HANDLER_H