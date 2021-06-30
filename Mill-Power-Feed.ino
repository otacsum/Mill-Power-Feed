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
#define STEPSPERREV 800 // Quarter-stepping (200 full steps per rev)
#define REVSPERINCH 20 // 2:1 Pulley Reduction, 10 screw turns per inch

// Imperial milling speeds defined in IPM, to be reduced to step pulses.
// This will be replaced with the rotary encoded values.
const int DesiredInchesPerMin = 1;

bool moveLeftEnabled = false;
bool moveRightEnabled = false;
bool stepperEnabled = false;

// Timing and pulse variables
unsigned long curMillis;
unsigned long prevStepMillis = 0;
unsigned long millisBetweenSteps = 10; // milliseconds
int pulseWidthMicroseconds = 10;

//Create new instance of a stepper controller
//1 = bipolar motor on dedicated driver
//AccelStepper stepMotor(1, PULSE, DIRECTION); 

// Calculate steps per second based on desired speed and driver constants.
int stepsPerSec() {
    int revsPerMin = DesiredInchesPerMin * REVSPERINCH;
    int stepsPerMin = revsPerMin * STEPSPERREV;
    // This isn't CNC, we're not measuirng distances just speed, so lost steps aren't relevant.
    float stepsPerSec = float(stepsPerMin / 60);

    return stepsPerSec;
}


void setup() {
    // Set pin default states
	pinModeFast(ENABLE, OUTPUT);
	pinModeFast(DIRECTION, OUTPUT);
	pinModeFast(MOVELEFT, INPUT);
	pinModeFast(MOVERIGHT, INPUT);
    pinModeFast(LED_BUILTIN, OUTPUT);
	
    // Pull the enable pin high to disable the driver by default
	digitalWriteFast(ENABLE, LOW);

	// Pull the direction pin low to set the default direction
	digitalWriteFast(DIRECTION, LOW);

    // Turn off the status LED for troubleshooting
    digitalWriteFast(LED_BUILTIN, LOW);


    Serial.begin(9600);
    Serial.println("Power Feed Enabled...");
}


void loop() { 
    
    curMillis = millis();
    readThreeWaySwitch();
    actOnSwitch();
    
}

void readThreeWaySwitch() {
    
    moveLeftEnabled = false;
    moveRightEnabled = false;
    
    if (digitalReadFast(MOVERIGHT) == HIGH) {
        enableStepper("Right (positive X)");
        moveRightEnabled = true;
    }
    else if (digitalReadFast(MOVELEFT) == HIGH) {
        enableStepper("Left (negative X)");
        moveLeftEnabled = true;
    }
    else {  // Disable the motor when the switch is centered.
        disableStepper();
    }
}

//Only toggle the state if it's disabled. Used for debounce
void enableStepper(String direction) {
    if (!stepperEnabled) {
        //delay(100); //Debounce on state change, for bouncy switches
        stepperEnabled = true;
        digitalWriteFast(ENABLE, LOW); // Enable the driver
        digitalWriteFast(LED_BUILTIN, HIGH);
        Serial.println("Moving" + direction);
    }
}

//Only toggle the state if it's enabled. Used for debounce
void disableStepper() {
    if (stepperEnabled) {
        delay(100); //Debounce on state change, for bouncy switches
        stepperEnabled = false;
        //digitalWriteFast(ENABLE, HIGH);
        digitalWriteFast(LED_BUILTIN, LOW);
        Serial.println();
        Serial.println("Motor Stopped");
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
    if (curMillis - prevStepMillis >= millisBetweenSteps) {
            // next 2 lines changed 28 Nov 2018
        //prevStepMillis += millisBetweenSteps;
        prevStepMillis = curMillis;
        digitalWriteFast(PULSE, HIGH);
        Serial.print(".");
        delayMicroseconds(pulseWidthMicroseconds); // this line is probably unnecessary?
        digitalWriteFast(PULSE, LOW);
    }
}