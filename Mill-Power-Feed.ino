// Hardware and user config parameters.
#include "configuration.h"

// Faster implementation of the pin states for fast stepper movement.
#include <digitalWriteFast.h> 

/**
* Fast stepper class for driving the power feed.
* ----------------------------------------------
*
* Unsigned long and long ints are often used due to the large numbers
* used in microsecond integer math.  Since this is not CNC precision, we
* are not worried about remainders or floats, since the math for these is so
* expensive in contrast to integer math.
* All to ensure the motor can rapid quickly, as much as 1,200 RPM or higher
* even when microstepping (within some limitations.)
*/
class FastStepper {

    private: 
        // Timing and pulse variables
        unsigned long prevMicros = 0;
        long microsPerStep; 
        long maxStepsPerSec;

        // Acceleration parameters
        unsigned long prevMillis = 0;
        long currentStepsPerSec = 0;
        long setStepsPerSec = 0;
        long startingStepsPerSec = 200;

        // Default states
        bool stepperEnabled = false;
        int maxInchesPerMin;

        // Time standards
        long secondsPerMin = 60;
        long microsPerSec = 1000000;

        // Hardware states from config
        int revolutionsPerInch;
        int stepsPerRevolution;

        // Stepper Driver Pins
        // [0] = PULSE
        // [1] = DIRECTION
        // [2] = ENABLE
        int controlPins[3];


    public:
        // Timing and acceleration timers
        unsigned long curMicros;
        unsigned long curMillis;

        // Velocity & Accel States
        float currentInchesPerMin;



    //Constructor
    FastStepper(int maxIPM, int revsPerInch, int stepsPerRev) {
        this->maxInchesPerMin = maxIPM;
        this->revolutionsPerInch = revsPerInch;
        this->stepsPerRevolution = stepsPerRev;
        this->maxStepsPerSec = this->getMaxStepsPerSec(maxInchesPerMin);
    }

    private:

        long getMaxStepsPerSec(float maxIPM) {
            this->maxStepsPerSec = this->calcStepsPerSec(maxIPM);
        }

        long calcStepsPerSec(float inchesPerMin) {
            // Using minutes because the truncated remainders of 
            // larger numbers are less significant.
            unsigned long RPM = inchesPerMin * this->revolutionsPerInch;
            unsigned long stepsPerMin = RPM * this->stepsPerRevolution;
            unsigned long stepsPerSec = stepsPerMin / this->secondsPerMin;

            return stepsPerSec;
        }
        
        unsigned long microsBetweenSteps() {
            this->microsPerSec / this->currentStepsPerSec;
        }

        void accelerateStepper() {
            if (this->curMillis - this->prevMillis >= accelInterval) { 
                //Increment timers any time we accelerate
                this->prevMillis = this->curMillis;

                // Accelerate until maxed
                if (this->currentStepsPerSec > this->setStepsPerSec) {
                    // we overshot slightly on the last loop, correct it
                    this->currentStepsPerSec = this->setStepsPerSec;
                }
                else {
                    this->currentStepsPerSec += accelRate;
                }
                
                this->microsBetweenSteps();
            }
        }

        void decelerateStepper() {
            if (this->curMillis - this->prevMillis >= accelInterval) { 
                //Increment timers any time we accelerate
                this->prevMillis = this->curMillis;

                // Decelerate until set speed or 0
                if (this->currentStepsPerSec < this->setStepsPerSec) {
                    // we overshot slightly on the last loop, correct it
                    this->currentStepsPerSec = this->setStepsPerSec;
                }
                else {
                    this->currentStepsPerSec -= accelRate;
                }
                
                this->microsBetweenSteps();
            }
        }

        void step() {
            // If the elapsed time since last step is enough, step again
            if (this->curMicros - this->prevMicros 
                        >= 
                        this->microsPerStep - calibrationMicros) {
                //Increment timers any time we step
                this->prevMicros = this->curMicros;

                //Pulse the driver
                digitalWriteFast(this->controlPins[0], HIGH);
                delayMicroseconds(pulseWidthMicroseconds);
                digitalWriteFast(this->controlPins[0], LOW);
            }
        }

    public:

        void begin(int pins[]) {
            // Set pin default states
            for (int i = 0; i < 3; i++) {
                pinModeFast(pins[i], OUTPUT);
                this->controlPins[i] = pins[i];
            }
        }

        void setSpeed(float inchesPerMin) {
            this->setStepsPerSec = this->calcStepsPerSec(inchesPerMin);
        }

        void setDirection(char direction) {
            if (direction == "right") {
                digitalWriteFast(this->controlPins[1], HIGH);
            }
            else {  // "left" or any invalid value deenergizes the direction pin
                digitalWriteFast(this->controlPins[1], LOW);
            }
        }

        void run() {
            if (!this->stepperEnabled) {
                delay(100); //Debounce on state change, for bouncy switches
                this->stepperEnabled = true;
                this->currentStepsPerSec = this->startingStepsPerSec; // Set minimum startup speed
                digitalWriteFast(this->controlPins[2], LOW); // Enable the driver
            }

            // If it's not already at the set speed, keep accelerating
            if (this->currentStepsPerSec < this->setStepsPerSec) {
                this->accelerateStepper();      
            }
            // If a new slower speed was entered, decelerate to it
            else if (this->currentStepsPerSec > this->setStepsPerSec) {
                this->decelerateStepper();
            }
                
            this->step();           
        }

        void stop() {
            if (this->stepperEnabled) {
                delay(100); //Debounce on state change, for bouncy switches
                this->stepperEnabled = false;
                this->setStepsPerSec = 0;
            }
            
            // If it's still moving, decelerate to 0
            if (this->currentStepsPerSec > this->setStepsPerSec) {
                this->decelerateStepper();
                this->step();
            }
            else {
                digitalWriteFast(this->controlPins[2], HIGH); // Disable the driver
                digitalWriteFast(this->controlPins[0], LOW); // Make sure the pulse pin is off
            } 
        }
};

FastStepper feedMotor(MAXINCHESPERMIN, REVSPERINCH, STEPSPERREV);

void setup() {
    //Initialize the pin outputs
    feedMotor.begin(controlPins);

    // Set pin default states
	pinModeFast(MOVELEFT_PIN, INPUT);
	pinModeFast(MOVERIGHT_PIN, INPUT);

	pinModeFast(RAPID_PIN, INPUT);
	
    // Pull the enable pin high to disable the driver by default
	digitalWriteFast(ENABLE_PIN, HIGH);

	// Pull the direction pin low to set the default direction
	digitalWriteFast(DIRECTION_PIN, LOW);

    // Pull the pulse pin low to set the default state
	digitalWriteFast(PULSE_PIN, LOW);

    feedMotor.setSpeed(MAXINCHESPERMIN);
}


void loop() { 
    feedMotor.curMicros = micros();
    feedMotor.curMillis = millis();
    readSwitches();
}

void readSwitches() {    
    if (digitalReadFast(MOVERIGHT_PIN) == HIGH) {
        feedMotor.setDirection("right");
        feedMotor.run();
    }
    else if (digitalReadFast(MOVELEFT_PIN) == HIGH) {
        feedMotor.setDirection("left");
        feedMotor.run();
    }
    else {  // Disable the motor when the switch is centered and the rapid isn't pressed.
        feedMotor.stop();
    }
}