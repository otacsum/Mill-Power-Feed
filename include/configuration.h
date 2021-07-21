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
#define PULSE_PIN 5
#define DIRECTION_PIN 6
#define ENABLE_PIN 7

// Pins used for direction signals
#define MOVELEFT_PIN 8
#define MOVERIGHT_PIN 9

// Pin used for rapids signal
#define RAPID_PIN 10

// LCD Pins
#define rs_PIN 48
#define lcdEnable_PIN 49
#define d4_PIN 50
#define d5_PIN 51
#define d6_PIN 52
#define d7_PIN 53


//Stepper Driver Configuration Values
//const unsigned long STEPSPERREV = 200; // Full-stepping (200 full steps per rev) 2x precision w/ 2:1 pulley
const unsigned long STEPSPERREV = 400; // Half-stepping (200 full steps => 400 half steps per rev) 4x precision w/ 2:1 pulley
//const unsigned long STEPSPERREV = 800; // Quarter-stepping (200 full steps => 800 quarter steps per rev) 8x precision w/ 2:1 pulley
const unsigned long REVSPERINCH = 20; // 2:1 Pulley Reduction, 10 screw turns per inch

// Imperial milling speeds defined in IPM, to be reduced to step pulses.
// This is the maximum rate that can be programmed in using the rotary encoder and...
// also the maximum speed that will be achieved when traversing in rapid mode.
const float MAXINCHESPERMIN = 36.00;
const float SPEEDINCREMENT  = 0.25; // Inch/Min per step of the rotary encoder.




/********* ADVANCED CONFIGURATION SETTINGS **********/

// Non-blocking delay in millis for debouncing switch state change
const unsigned long DEBOUNCEMILLISMOMENTARY = 25;
const unsigned long DEBOUNCEMILLIS3WAY = 50;

// Delay in millis for infrequent reading of switches to help motor run more efficiently.
const unsigned long SWITCHREADDELAY = 50;

// Motor pulse width, make this as long as possible without impacting your RPMs
// Sending the driver's minimum requirement will reduce torque, even though it's not supposed to.
const unsigned int pulseWidthMicroseconds = 50; 

// Acceleration Params (linear acceleration)
const float MININCHESPERMINUTE = 0.1;
const long accelInterval = 10;  // Millis between increasing velocity
const long accelRate = 20; // Steps increased per accelInterval


// Rotary Encoder Increment Steps per Detent
// May vary per encoder, check with a test script first or adjust if
// Your increments are off by a multiple of N
const int encoderStepsPerDetent = 4;






/*******************************************************************************
 *******************************************************************************
 *******************************************************************************
 * DO NOT MODIFY BELOW THIS LINE
 *******************************************************************************
 *******************************************************************************
 *******************************************************************************/
 





const int PRESSED = LOW;
const int UNPRESSED = HIGH;

// Time standards
const long secondsPerMin = 60;
const unsigned long microsPerSec = 1000000;

// Minimum delay between motor pulses, sanity check for runaway math.
// == 1M / max steps per sec.
const unsigned long maxStepsPerSec = (MAXINCHESPERMIN * REVSPERINCH * STEPSPERREV) / secondsPerMin;
const unsigned long minMicrosPerStep = microsPerSec / maxStepsPerSec;

// Max delay between motor pulses, basically accel jerk limit from zero.
const unsigned long startStepsPerSec = (MININCHESPERMINUTE * REVSPERINCH * STEPSPERREV) / secondsPerMin;
const unsigned long startMicrosPerStep = microsPerSec / startStepsPerSec;

// Velocity value set by the rotary encoder
volatile float encodedInchesPerMin = 0.00;

// Object initialization arrays
int stepperControlPins[3] = {PULSE_PIN, DIRECTION_PIN, ENABLE_PIN};
int threeWayPins[2] = {MOVELEFT_PIN, MOVERIGHT_PIN};

// Rotary encoder position at boot
long oldEncoderPosition  = -999999;

// Calculated max rotary encoder readings for Max Speed
const long maxEncoderPosition = (MAXINCHESPERMIN / SPEEDINCREMENT) * encoderStepsPerDetent;