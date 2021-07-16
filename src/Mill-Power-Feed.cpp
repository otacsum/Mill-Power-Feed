// Core Arduino libraries
#include <Arduino.h>
//#include <LiquidCrystal.h>

// Faster implementation of reading/writing pin states for fast stepper movement.
#include <digitalWriteFast.h> 

/* Rotary Encoder Library: http://www.pjrc.com/teensy/td_libs_Encoder.html */
#include <Encoder.h>

// Hardware and user config parameters.
#include "configuration.h"

// Homegrown stepper driver for fast stepping / microstepping.
#include <FastStepper.h>

// Controller for a SPDT switch for controlling direction
#include <ThreeWaySwitch.h>

// Controller for a momentary SPST N/O switch for rapid function
#include <MomentarySwitch.h>

//LiquidCrystal lcd(1, 2, 4, 5, 6, 7); // Creates an LCD object. Parameters: (rs, enable, d4, d5, d6, d7) 

FastStepper stepper(MAXINCHESPERMIN, REVSPERINCH, STEPSPERREV);
ThreeWaySwitch directionSwitch(&stepper, DEBOUNCEMILLIS3WAY);
MomentarySwitch rapidButton(&stepper, DEBOUNCEMILLISMOMENTARY);

// Custom Rotary Encoder Implementation.  Must be included AFTER FastStepper is initialized.
#include <RotaryEncoder.h>

void setup() {
    if (DEBUG) { // Log Events to Serial Monitor
        Serial.begin(9600);  
    }

    //lcd.begin(16,2); // Initializes the interface to the LCD screen, and specifies the dimensions (width and height) of the display } 

    // Set defaults
    stepper.setSpeed(encodedInchesPerMin);

    // Initialize the pin outputs/inputs
    stepper.begin(stepperControlPins);
    directionSwitch.begin(threeWayPins);
    rapidButton.begin(RAPID_PIN);

    // Set up Rotary Encoder Interrupts
    //beginRotaryEncoder();
}


void loop() { 
    directionSwitch.run();
    directionSwitch.read();
    rapidButton.read();
    readRotaryEncoder();
}