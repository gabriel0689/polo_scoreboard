#ifndef DISPLAY_SETUP_H
#define DISPLAY_SETUP_H

#include <TFT_eSPI.h>

// External declarations
extern TFT_eSPI tft;

// Function declarations
void initDisplay();
void displayMessage(String message);

#endif 