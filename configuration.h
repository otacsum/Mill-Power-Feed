// Set truthy to turn on Serial logging for debugging.  
// Note the serial log blocking time will affect RPMs,
// serial logging cannot be used to diagnose RPM inaccuracies, 
// you must use an external tachometer.
extern bool DEBUG = false;  

// Pins used for stepper control signals
#define PULSE_PIN 5
#define DIRECTION_PIN 6
#define ENABLE_PIN 7

// Pins used for direction signals
#define MOVELEFT_PIN 8
#define MOVERIGHT_PIN 9

// Pin used for rapids signal
#define RAPID_PIN 10

//Stepper Driver Configuration Values
#define STEPSPERREV 400 // Half-stepping (200 full steps => 400 half steps per rev)
#define REVSPERINCH 20 // 2:1 Pulley Reduction, 10 screw turns per inch

// Imperial milling speeds defined in IPM, to be reduced to step pulses.
// This is the maximum rate that can be programmed in using the rotary encoder and...
// also the maximum speed that will be achieved when traversing in rapid mode.
#define MAXINCHESPERMIN 35

// Non-blocking delay in millis for debouncing switch state change
#define DEBOUNCEMILLISMOMENTARY 50
#define DEBOUNCEMILLIS3WAY 20

// Motor pulse width, make this as long as possible without impacting your RPMs
// Sending the driver's minimum requirement will reduce torque.
int pulseWidthMicroseconds = 50; 

// Pulse width delay is a blocking function, so this helps counter it for more accurate speeds.
// Higher numbers = higher speeds
int calibrationMicros = 12;

// Acceleration Params (linear acceleration)
const long accelInterval = 5;  // Millis between increasing velocity
const long accelRate = 20; // Steps increased per accelInterval












/**
 * DO NOT MODIFY BELOW THIS LINE
 * DO NOT MODIFY BELOW THIS LINE
 * DO NOT MODIFY BELOW THIS LINE
 */

int stepperControlPins[3] = {PULSE_PIN, DIRECTION_PIN, ENABLE_PIN};
int threeWayPins[2] = {MOVELEFT_PIN, MOVERIGHT_PIN};