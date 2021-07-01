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
const int InchesPerMin = 30;

bool moveLeftEnabled = false;
bool moveRightEnabled = false;
bool stepperEnabled = false;

// Timing and pulse variables
unsigned long curMicros;
unsigned long prevStepMicros = 0;
int microsPerStep; 
int pulseWidthMicroseconds = 4;


unsigned long microsBetweenSteps() {
    unsigned long RPM = InchesPerMin * REVSPERINCH;
    unsigned long stepsPerMin = RPM * STEPSPERREV;
    unsigned long microsPerMin = 60000000; // Magic number, but it never changes.
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


    Serial.begin(9600);
    Serial.println("Power Feed Enabled...");
    Serial.print("Micros Between Steps: "); Serial.println(microsPerStep);
}


void loop() { 
    curMicros = micros();
    readThreeWaySwitch();
}

void readThreeWaySwitch() {    
    if (digitalReadFast(MOVERIGHT) == HIGH) {
        moveRightEnabled = true;
        moveLeftEnabled = false;
        enableStepper("Right (positive X)");
    }
    else if (digitalReadFast(MOVELEFT) == HIGH) {
        moveLeftEnabled = true;
        moveRightEnabled = false;
        enableStepper("Left (negative X)");
    }
    else {  // Disable the motor when the switch is centered.
        moveLeftEnabled = false;
        moveRightEnabled = false;
        disableStepper();
    }
}

//Only toggle the state if it's disabled. Used for debounce
void enableStepper(String direction) {
    if (!stepperEnabled) {
        delay(100); //Debounce on state change, for bouncy switches
        stepperEnabled = true;
        digitalWriteFast(ENABLE, LOW); // Enable the driver
        Serial.println("Moving " + direction);
    }

    actOnSwitch();
}

//Only toggle the state if it's enabled. Used for debounce
void disableStepper() {
    if (stepperEnabled) {
        delay(100); //Debounce on state change, for bouncy switches
        stepperEnabled = false;
        digitalWriteFast(ENABLE, HIGH);
        digitalWriteFast(LED_BUILTIN, LOW);
        digitalWriteFast(PULSE, LOW);
        Serial.println();
        Serial.println("Motor Stopped");
        Serial.println();
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
    if (curMicros - prevStepMicros >= microsPerStep) {
        prevStepMicros = curMicros;
        digitalWriteFast(PULSE, HIGH);
        delayMicroseconds(pulseWidthMicroseconds);
        digitalWriteFast(PULSE, LOW);
    }
}