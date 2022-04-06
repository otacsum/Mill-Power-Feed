// Core Arduino libraries
#include <Arduino.h>
#include <FastAccelStepper.h>
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

// Create instances of the fast stepper objects.
// Documentation at https://github.com/gin66/FastAccelStepper
FastAccelStepperEngine engine = FastAccelStepperEngine();
FastAccelStepper *stepper = NULL;

// Stepper utilities to compliment FastAccelStepper and other button states
#include <FastStepperUtils.h>
FastStepperUtils stepperUtils;

// Controller for a SPDT switch for controlling direction
#include <ThreeWaySwitch.h>
ThreeWaySwitch directionSwitch;

// Controller for a momentary SPST N/O switch for rapid function
#include <MomentarySwitch.h>
MomentarySwitch encoderButton(DEBOUNCEMILLISMOMENTARY, 1);
MomentarySwitch rapidButton(DEBOUNCEMILLISMOMENTARY, 0);


Encoder rotaryEncoder(rotaryPinA, rotaryPinB);
#include <RotaryEncoder.h> // Custom rotary encoder controller.  

void setup() {
    if (DEBUG) { // Log Events to Serial Monitor
        Serial.begin(9600);  
    }
    
    // Initializes the interface to the LCD screen, and specifies the dimensions (width and height) of the display
    lcd.begin(16,2); 

    // Initialize stepper motor stuff
    engine.init();
    stepper = engine.stepperConnectToPin(PULSE_PIN);
    if (stepper) {
        //stepper->setDirectionPin(DIRECTION_PIN);
        pinModeFast(DIRECTION_PIN, OUTPUT);
        stepper->setEnablePin(ENABLE_PIN);
        stepper->setAutoEnable(true);
        stepper->setDelayToEnable(50);
        stepper->setDelayToDisable(1000);
        stepper->setAcceleration(5000); // Steps/sec^2
        
        // Disable motors @ boot
        stepper->moveTo(1);
        stepper->stopMove();
    }

    // Initialize the pin outputs/inputs and run setup tasks
    directionSwitch.begin(threeWayPins);
    rapidButton.begin(RAPID_PIN);
    encoderButton.begin(rotaryMomentaryPin);

    // Everything is set up, let's go!
    lcdMessage.welcomeMessage();
}


void loop() { 
    directionSwitch.read();
    rapidButton.read();
    encoderButton.read();

    readRotaryEncoder(); // Abstraction to simplify encoder readings and translation to speed/events.
}
