/********  DEBUGGING  ********
 * Set truthy to turn on Serial logging for debugging.  
 * Notes:
 *  The serial log blocking time will affect RPMs &
 *  rapid changes to speed using the rotary encoder will
 *  cause the motor to halt and miss steps.
 *  Also, serial logging cannot be used to diagnose RPM inaccuracies, 
 *  you must use an external tachometer with DEBUG = false to
 *  diagnose and tune RPMs.
 */ 
bool DEBUG = false;

// Pins used for rotary encoder.  Depending on your board you 
// might need to specifically use these two pins for interrupts.  Change with caution.
// See: https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/
#define rotaryPinA 2 // Our first hardware interrupt pin is digital pin 2
#define rotaryPinB 3 // Our second hardware interrupt pin is digital pin 3
#define rotaryMomentaryPin 4 // When pressing in on the rotary knob

// Pins used for stepper control signals
// This is "timer 4" on a Mega 2560, only these three pins (or another timer triplet may be used)
#define PULSE_PIN 6
#define DIRECTION_PIN 7
#define ENABLE_PIN 8

// Pins used for direction signals
#define MOVELEFT_PIN 9
#define MOVERIGHT_PIN 10

// Pin used for rapids signal
#define RAPID_PIN 11

// LCD Pins
#define rs_PIN 48
#define lcdEnable_PIN 49
#define d4_PIN 50
#define d5_PIN 51
#define d6_PIN 52
#define d7_PIN 53


//Stepper Driver Configuration Values
//#define STEPSPERREV 200 // Full-stepping (200 full steps per rev) 2x precision w/ 2:1 pulley
//#define STEPSPERREV 400 // Half-stepping (200 full steps => 400 half steps per rev) 4x precision w/ 2:1 pulley
long STEPSPERREV = 800; // Quarter-stepping (200 full steps => 800 quarter steps per rev) 8x precision w/ 2:1 pulley
int REVSPERINCH = 20; // 2:1 Pulley Reduction, 10 screw turns per inch

// Imperial milling speeds defined in IPM, to be reduced to step pulses.
// This is the maximum rate that can be programmed in using the rotary encoder and...
// also the maximum speed that will be achieved when traversing in rapid mode.
float MAXINCHESPERMIN = 36.00;
float SPEEDINCREMENT = 0.25; // Inch/Min per step of the rotary encoder.




/********* ADVANCED CONFIGURATION SETTINGS **********/

// Non-blocking delay in millis for debouncing switch state change
const unsigned long DEBOUNCEMILLISMOMENTARY = 20;
const unsigned long DEBOUNCEMILLIS3WAY = 20;

// Delay in millis for infrequent reading of switches to help motor run more efficiently.
const unsigned long SWITCHREADDELAY = 20;

// Rotary Encoder Increment Steps per Detent
// May vary per encoder, check with a test script first or adjust if
// Your increments are off by a multiple of N
int encoderStepsPerDetent = 4;






/*******************************************************************************
 *******************************************************************************
 *******************************************************************************
 * DO NOT MODIFY BELOW THIS LINE
 *******************************************************************************
 *******************************************************************************
 *******************************************************************************/
 





int PRESSED = LOW;
int UNPRESSED = HIGH;

// Velocity value set by the rotary encoder
volatile float encodedInchesPerMin = 0.00;

// Object initialization arrays
int threeWayPins[2] = {MOVELEFT_PIN, MOVERIGHT_PIN};

// Rotary encoder position at boot
long oldEncoderPosition  = -999999;

// Calculated max rotary encoder readings for Max Speed
long maxEncoderPosition = (MAXINCHESPERMIN / SPEEDINCREMENT) * encoderStepsPerDetent;