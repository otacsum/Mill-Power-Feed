// Core Arduino libraries
#include <Arduino.h>
#include <LiquidCrystal.h>

// Faster implementation of reading/writing pin states for fast stepper movement.
// Library uses direct port manipulation, and is nearly as fast as the obscure
// direct port manipulation syntax, but easier to read.
#include <digitalWriteFast.h> 

/* Rotary Encoder Library: http://www.pjrc.com/teensy/td_libs_Encoder.html */
#include <Encoder.h>

// Hardware and user config parameters.
#include <configuration.h>

// Create instances of #all the objects
LiquidCrystal lcd(rs_PIN, lcdEnable_PIN, d4_PIN, d5_PIN, d6_PIN, d7_PIN);
#include <LCDMessage.h> // Custom LCD events to minimize duplicate code.  
LCDMessage lcdMessage;

// Homegrown stepper driver for fast stepping / microstepping.
#include <FastStepper.h>
FastStepper stepper(MAXINCHESPERMIN, REVSPERINCH, STEPSPERREV);

// Controller for a SPDT switch for controlling direction
#include <ThreeWaySwitch.h>
ThreeWaySwitch directionSwitch(&stepper, DEBOUNCEMILLIS3WAY);

// Controller for a momentary SPST N/O switch for rapid function
#include <MomentarySwitch.h>
MomentarySwitch rapidButton(&stepper, DEBOUNCEMILLISMOMENTARY, 0);
MomentarySwitch encoderButton(&stepper, DEBOUNCEMILLISMOMENTARY, 1);


Encoder rotaryEncoder(rotaryPinA, rotaryPinB);
#include <RotaryEncoder.h> // Custom rotary encoder controller.  

void setup() {
    if (DEBUG) { // Log Events to Serial Monitor
        Serial.begin(9600);  
    }
    
    // Initializes the interface to the LCD screen, and specifies the dimensions (width and height) of the display
    lcd.begin(16,2); 

    // Initialize the pin outputs/inputs
    stepper.begin(stepperControlPins);
    directionSwitch.begin(threeWayPins);
    lcdMessage.welcomeMessage();
    rapidButton.begin(RAPID_PIN);
    encoderButton.begin(rotaryMomentaryPin);
}


void loop() { 
    stepper.step(); // Always update the step timer, even when not running
    directionSwitch.run();
    directionSwitch.read();
    rapidButton.read();
    encoderButton.read();

    readRotaryEncoder(); // Abstraction to simplify encoder readings and translation to speed/events.
}