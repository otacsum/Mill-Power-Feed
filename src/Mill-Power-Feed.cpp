// Core Arduino libraries
#include <Arduino.h>
// Faster implementation of the pin states for fast stepper movement.
#include <digitalWriteFast.h> 

// Hardware and user config parameters.
#include "configuration.h"

// Homegrown stepper driver for fast stepping / microstepping.
#include <FastStepper.h>

// Controller for a SPDT switch for controlling direction
#include <ThreeWaySwitch.h>

// Controller for a momentary SPST N/O switch for rapid function
#include <MomentarySwitch.h>

FastStepper stepper(MAXINCHESPERMIN, REVSPERINCH, STEPSPERREV);
ThreeWaySwitch directionSwitch(&stepper, DEBOUNCEMILLIS3WAY);
MomentarySwitch rapidButton(&stepper, DEBOUNCEMILLISMOMENTARY);

// Interrupt-Based Rotary Encoder Functions
#include <RotaryEncoder.h>

void setup() {
    if (DEBUG) { // Log Events to Serial Monitor
        Serial.begin(9600);  
    }

    // Set defaults
    stepper.setSpeed(encodedInchesPerMin);

    // Initialize the pin outputs/inputs
    stepper.begin(stepperControlPins);
    directionSwitch.begin(threeWayPins);
    rapidButton.begin(RAPID_PIN);

    // Set up Rotary Encoder Interrupts
    beginRotaryEncoder();
}


void loop() { 
    directionSwitch.run();
    directionSwitch.read();
    rapidButton.read();
    //logRotaryEncoder(); Disabled, unneeded at this time.
}