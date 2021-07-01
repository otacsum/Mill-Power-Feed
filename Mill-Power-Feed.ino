//Faster implementation of the pin states for fast stepper movement.
#include <digitalWriteFast.h> 

// Pins used for stepper control signals
#define PULSE 5
#define DIRECTION 6
#define ENABLE 7

// Pins used for direction signals
#define MOVELEFT 8
#define MOVERIGHT 9

//Stepper Driver Configuration Values
#define STEPSPERREV 400 // Half-stepping (200 full steps => 400 half steps per rev)
#define REVSPERINCH 20 // 2:1 Pulley Reduction, 10 screw turns per inch

// Imperial milling speeds defined in IPM, to be reduced to step pulses.
// This will be replaced with the rotary encoded values.
const long InchesPerMin = 60;

bool moveLeftEnabled = false;
bool moveRightEnabled = false;
bool stepperEnabled = false;

// Timing and pulse variables
unsigned long curMicros;
unsigned long prevStepMicros = 0;
long microsPerStep; 
int pulseWidthMicroseconds = 5;

// Acceleration Params
long stepsPerSec = 0;
long maxStepsPerSec;
unsigned long curMillis;
unsigned long prevMillis = 0;
const long accelInterval = 5;  // Millis
const long accelRate = 20; // Steps per accel interval



unsigned long microsBetweenSteps() {
    unsigned long RPM = InchesPerMin * REVSPERINCH;
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
	pinModeFast(PULSE, OUTPUT);
	pinModeFast(DIRECTION, OUTPUT);
	pinModeFast(ENABLE, OUTPUT);

	pinModeFast(MOVELEFT, INPUT);
	pinModeFast(MOVERIGHT, INPUT);

    pinModeFast(LED_BUILTIN, OUTPUT);
	
    // Pull the enable pin high to disable the driver by default
	digitalWriteFast(ENABLE, HIGH);

	// Pull the direction pin low to set the default direction
	digitalWriteFast(DIRECTION, LOW);

    // Pull the pulse pin low to set the default state
	digitalWriteFast(PULSE, LOW);

    // Turn off the status LED for troubleshooting
    digitalWriteFast(LED_BUILTIN, LOW);

    microsPerStep = microsBetweenSteps();
}


void loop() { 
    curMicros = micros();
    curMillis = millis();
    readThreeWaySwitch();
}

void readThreeWaySwitch() {    
    if (digitalReadFast(MOVERIGHT) == HIGH) {
        moveRightEnabled = true;
        moveLeftEnabled = false;
        enableStepper();
    }
    else if (digitalReadFast(MOVELEFT) == HIGH) {
        moveLeftEnabled = true;
        moveRightEnabled = false;
        enableStepper();
    }
    else {  // Disable the motor when the switch is centered.
        disableStepper();
    }
}

//Only toggle the state if it's disabled. Used for debounce
void enableStepper() {
    if (!stepperEnabled) {
        delay(100); //Debounce on state change, for bouncy switches
        stepperEnabled = true; // Set state of switch
        stepsPerSec = 1; // Acceleration from zero on state change.
        digitalWriteFast(ENABLE, LOW); // Enable the driver
    }

    //Acceleration
    if (stepsPerSec < maxStepsPerSec) {  // If it's not already maxed out
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
        digitalWriteFast(ENABLE, HIGH);
        digitalWriteFast(LED_BUILTIN, LOW);
        digitalWriteFast(PULSE, LOW);
        moveLeftEnabled = false;
        moveRightEnabled = false;
    }
}

void actOnSwitch() {
    if (moveRightEnabled == true) {
        digitalWriteFast(DIRECTION, LOW);
        singleStep();
    }
    if (moveLeftEnabled == true) {
        digitalWriteFast(DIRECTION, HIGH);
        singleStep();
    }
}

void singleStep() {
    if (curMicros - prevStepMicros >= microsPerStep - pulseWidthMicroseconds) {

        //Increment timers any time we step
        prevStepMicros = curMicros;

        //Pulse the driver
        digitalWriteFast(PULSE, HIGH);
        delayMicroseconds(pulseWidthMicroseconds);
        digitalWriteFast(PULSE, LOW);
    }
}