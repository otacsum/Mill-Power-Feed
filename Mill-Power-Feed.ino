//Faster implementation of the pin states for fast stepper movement.
#include <digitalWriteFast.h> 

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
// This will be replaced with the rotary encoded values.
const long maxInchesPerMin = 35;

bool moveLeftEnabled = false;
bool moveRightEnabled = false;
bool stepperEnabled = false;

// Timing and pulse variables
unsigned long curMicros;
unsigned long prevStepMicros = 0;
long microsPerStep; 
int pulseWidthMicroseconds = 50; //Pulse width, minimum sent to driver will affect torque negatively.

// Pulse width is a blocking function, so this helps counter it for accurate speeds.
// Higher numbers = higher speeds
int calibrationMicros = 8;  

// Acceleration Params
long stepsPerSec = 0;
long maxStepsPerSec;
unsigned long curMillis;
unsigned long prevMillis = 0;
const long accelInterval = 5;  // Millis
const long accelRate = 20; // Steps per accel interval



unsigned long microsBetweenSteps() {
    unsigned long RPM = maxInchesPerMin * REVSPERINCH;
    //Using minutes because the truncated remainders are less significant.
    unsigned long stepsPerMin = RPM * STEPSPERREV;
    maxStepsPerSec = stepsPerMin / long(60);
    unsigned long microsPerMin = long(60000000); // Magic number, but it never changes.
    // This isn't CNC, we're not measuirng distances just speed, remainders aren't relevant.
    unsigned long microsPerStep = microsPerMin / stepsPerMin;

    return microsPerStep;
} 


void setup() {
    // Set pin default states
	pinModeFast(PULSE_PIN, OUTPUT);
	pinModeFast(DIRECTION_PIN, OUTPUT);
	pinModeFast(ENABLE_PIN, OUTPUT);

	pinModeFast(MOVELEFT_PIN, INPUT);
	pinModeFast(MOVERIGHT_PIN, INPUT);

	pinModeFast(RAPID_PIN, INPUT);

    pinModeFast(LED_BUILTIN, OUTPUT);
	
    // Pull the enable pin high to disable the driver by default
	digitalWriteFast(ENABLE_PIN, HIGH);

	// Pull the direction pin low to set the default direction
	digitalWriteFast(DIRECTION_PIN, LOW);

    // Pull the pulse pin low to set the default state
	digitalWriteFast(PULSE_PIN, LOW);

    // Turn off the status LED for troubleshooting
    digitalWriteFast(LED_BUILTIN, LOW);

    microsPerStep = microsBetweenSteps();
}


void loop() { 
    curMicros = micros();
    curMillis = millis();
    readSwitches();
}

void readSwitches() {    
    if (digitalReadFast(MOVERIGHT_PIN) == HIGH) {
        moveRightEnabled = true;
        moveLeftEnabled = false;
        enableStepper();
    }
    else if (digitalReadFast(MOVELEFT_PIN) == HIGH) {
        moveLeftEnabled = true;
        moveRightEnabled = false;
        enableStepper();
    }
    else {  // Disable the motor when the switch is centered and the rapid isn't pressed.
        disableStepper();
    }
}

//Only toggle the state if it's disabled. Used for debounce
void enableStepper() {
    if (!stepperEnabled) {
        delay(100); //Debounce on state change, for bouncy switches
        stepperEnabled = true; // Set state of switch
        stepsPerSec = 200; // Accelerate on state change.  Set starting value.
        digitalWriteFast(ENABLE_PIN, LOW); // Enable the driver
    }

    //Acceleration
    if (stepsPerSec < maxStepsPerSec) {  // If it's not already at the set speed
        if (curMillis - prevMillis >= accelInterval) { // Acceleration rate per second
            //Increment timers any time we accelerate
            prevMillis = curMillis;
            stepsPerSec += accelRate;
            long stepsPerMin = stepsPerSec * long(60);
            microsPerStep = long(60000000) / stepsPerMin;
            //Serial.print("Micros/Step: "); Serial.println(microsPerStep);
        }
    }
    else {
        microsPerStep = microsBetweenSteps();
    }

    actOnSwitch();
}

//Only toggle the state if it's enabled. Used for debounce
void disableStepper() {
    //Deceleration
    if (stepsPerSec > 0) {  // If it's not already maxed out
        if (curMillis - prevMillis >= accelInterval) { // Acceleration rate per second
            //Increment timers any time we accelerate
            prevMillis = curMillis;
            stepsPerSec -= accelRate;
            long stepsPerMin = stepsPerSec * long(60);
            microsPerStep = long(60000000) / stepsPerMin;
            //Serial.print("Micros/Step: "); Serial.println(microsPerStep);
        }
        actOnSwitch();
    }
    else {
        stepperEnabled = false;
        digitalWriteFast(ENABLE_PIN, HIGH);
        digitalWriteFast(LED_BUILTIN, LOW);
        digitalWriteFast(PULSE_PIN, LOW);
        moveLeftEnabled = false;
        moveRightEnabled = false;
    }
}

void actOnSwitch() {
    if (moveRightEnabled == true) {
        digitalWriteFast(DIRECTION_PIN, LOW);
        singleStep();
    }
    if (moveLeftEnabled == true) {
        digitalWriteFast(DIRECTION_PIN, HIGH);
        singleStep();
    }
}

void singleStep() {
    if (curMicros - prevStepMicros >= microsPerStep - calibrationMicros) {

        //Increment timers any time we step
        prevStepMicros = curMicros;

        //Pulse the driver
        digitalWriteFast(PULSE_PIN, HIGH);
        delayMicroseconds(pulseWidthMicroseconds);
        digitalWriteFast(PULSE_PIN, LOW);
    }
}