// Faster implementation of the pin states for fast stepper movement.
#include <digitalWriteFast.h> 

// Hardware and user config parameters.
#include "configuration.h"

// Stepper class
#include "FastStepper.h"

FastStepper feedMotor(maxInchesPerMin, REVSPERINCH, STEPSPERREV);

void setup() {
    // Set pin default states
	pinModeFast(PULSE_PIN, OUTPUT);
	pinModeFast(DIRECTION_PIN, OUTPUT);
	pinModeFast(ENABLE_PIN, OUTPUT);

	pinModeFast(MOVELEFT_PIN, INPUT);
	pinModeFast(MOVERIGHT_PIN, INPUT);

	pinModeFast(RAPID_PIN, INPUT);
	
    // Pull the enable pin high to disable the driver by default
	digitalWriteFast(ENABLE_PIN, HIGH);

	// Pull the direction pin low to set the default direction
	digitalWriteFast(DIRECTION_PIN, LOW);

    // Pull the pulse pin low to set the default state
	digitalWriteFast(PULSE_PIN, LOW);
}


void loop() { 
    feedMotor.curMicros = micros();
    feedMotor.curMillis = millis();
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